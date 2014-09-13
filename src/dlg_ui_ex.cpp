#include <sstream>
#include <Fl/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <curl/curl.h>
#include <locale.h>
#include <cstdlib>
#include "tracklayer.hpp"
#include "wgt_map.hpp"
#include "settings.hpp"
#include "gpsdclient.hpp"
#include "utils.hpp"
#include "unit.hpp"
#include "fluid/dlg_ui.hpp"

static dlg_ui *ui;

int main_ex(int argc, char* argv[])
{
    // Install signal handlers
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);

    // Setup gettext
    setlocale(LC_ALL, "");
    bindtextdomain("florb", LOCALEDIR);
    textdomain("florb");

    // Init CURL
    curl_global_init(CURL_GLOBAL_ALL);

    // Start the application
    Fl::lock();
    ui = new dlg_ui();
    ui->show(argc, argv);

    int ret = Fl::run();

    // delete the dialog, it is hidden already
    delete ui;

    // Deinit CURL
    curl_global_cleanup();

    return ret;
}

void sighandler_ex(int sig)
{
    // "Just die already!" approach: Clean up curl and get out of here. Sadly
    // the OS has to take care of memory deallocation from here on.
    curl_global_cleanup();
    exit(EXIT_SUCCESS);
}

void dlg_ui::update_statusbar_ex()
{
    florb::cfg_units cfgunits = florb::settings::get_instance()["units"].as<florb::cfg_units>();

    std::ostringstream ss; 

    ss << _("Zoom: ") << m_wgtmap->zoom();
    m_txtout_zoom->value(ss.str().c_str());

    florb::point2d<double> pos = m_wgtmap->mousepos();
    ss.precision(5);
    ss.setf(std::ios::fixed, std::ios::floatfield);

    ss.str("");
    ss << _("Lon: ") << pos.x() << "°";
    m_txtout_lon->value(ss.str().c_str());

    ss.str("");
    ss << _("Lat: ") << pos.y() << "°";
    m_txtout_lat->value(ss.str().c_str());

    florb::unit::length dst;
    switch (cfgunits.system_length())
    {
        case (florb::cfg_units::system::NAUTICAL):
            dst = florb::unit::length::SEA_MILE;
            break;
        case (florb::cfg_units::system::IMPERIAL):
            dst = florb::unit::length::ENGLISH_MILE;
            break;
        default:
            dst = florb::unit::length::KM;
            break;
    }

    ss.precision(2);
    ss.setf(std::ios::fixed, std::ios::floatfield);
    ss.str("");
    ss << _("Trip: ") << florb::unit::convert(florb::unit::length::KM, dst, m_wgtmap->gpx_trip()) << " " << florb::unit::sistr(dst);
    m_txtout_trip->value(ss.str().c_str());

    if (m_wgtmap->gpsd_connected())
    {
        switch(m_wgtmap->gpsd_mode())
        {
            case (florb::gpsdclient::FIX_2D):
                m_box_fix->color(FL_GREEN);
                m_box_fix->label("2D");
                break;
            case (florb::gpsdclient::FIX_3D):
                m_box_fix->color(FL_GREEN);
                m_box_fix->label("3D");
                break;
            default:
                m_box_fix->color(FL_YELLOW);
                m_box_fix->label("?");
                break;
        }
    } else
    {
        m_box_fix->color(FL_RED);
        m_box_fix->label(_("GPS"));
    }
}

bool dlg_ui::wgtmap_evt_notify_ex(const florb::wgt_map::event_notify *e)
{
    update_statusbar_ex();
    return true;
}

bool dlg_ui::wgtmap_evt_endselect_ex(const florb::wgt_map::event_endselect *e)
{
    if (!m_dlg_bulkdl)
        m_dlg_bulkdl = new dlg_bulkdl(m_wgtmap);
    
    m_dlg_bulkdl->show(e->vp());

    m_wgtmap->select_clear();

    return true;
}

void dlg_ui::create_ex(void)
{
    // Set the window icon
    florb::utils::set_window_icon(m_window);

    // Fluid 1.3 does not gettext the menuitems, do it manually here
    // File
    m_menuitem_file->label(_("File"));
    m_menuitem_file_opengpx->label(_("Open GPX"));
    m_menuitem_file_savegpx->label(_("Save GPX as"));
    m_menuitem_file_quit->label(_("Quit"));
    // Edit
    m_menuitem_edit->label(_("Edit"));
    m_menuitem_edit_settings->label(_("Settings"));
    m_menuitem_edit_search->label(_("Search"));
    // Track
    m_menuitem_track->label(_("Track"));
    m_menuitem_track_clear->label(_("Clear"));
    m_menuitem_track_editwaypoint->label(_("Edit waypoints"));
    m_menuitem_track_deletewaypoints->label(_("Delete waypoints"));
    m_menuitem_track_showwpmarkers->label(_("Show waypoint markers"));
    m_menuitem_track_garmindl->label(_("Download from device"));
    m_menuitem_track_garminul->label(_("Upload to device"));
    // GPSs
    m_menuitem_gpsd->label(_("GPSd"));
    m_menuitem_gpsd_gotocursor->label(_("Go to cursor"));
    m_menuitem_gpsd_recordtrack->label(_("Record track"));
    m_menuitem_gpsd_lockcursor->label(_("Lock to cursor"));
    // Help
    m_menuitem_help->label(_("Help"));
    m_menuitem_help_about->label(_("About"));

    // Populate the Basemap selector
    update_choice_map_ex();

    // Update statusbar
    update_statusbar_ex();

    // GPS status
    m_box_fix->color(FL_RED);
    m_box_fix->label(_("GPS"));

    // Show waypoint markers by default
    m_menuitem_track_showwpmarkers->set(); 
    m_wgtmap->gpx_showwpmarkers(true);

    // Start listening to florb::wgt_mapevents
    register_event_handler<dlg_ui, florb::wgt_map::event_notify>(this, &dlg_ui::wgtmap_evt_notify_ex);
    register_event_handler<dlg_ui, florb::wgt_map::event_endselect>(this, &dlg_ui::wgtmap_evt_endselect_ex);
    m_wgtmap->add_event_listener(this);
}

void dlg_ui::update_choice_map_ex(void)
{
    // Remember the currently selected item index if any, then clear the widget
    int idxbase = (m_choice_basemap->value() >= 0) ? m_choice_basemap->value() : 0;
    int idxover = (m_choice_overlay->value() >= 0) ? m_choice_overlay->value() : 0;
    
    m_choice_basemap->clear();
    m_choice_overlay->clear();
    m_choice_overlay->add(_("None"));

    // Get the configured tileservers from the florb::settings and populate the widgets
    florb::node section = florb::settings::get_instance()["tileservers"];
    for(florb::node::iterator it=section.begin(); it!=section.end(); ++it) {
        m_choice_basemap->add((*it).as<florb::cfg_tileserver>().name().c_str());
        m_choice_overlay->add((*it).as<florb::cfg_tileserver>().name().c_str());
    }

    // If there are any connfigured tileservers at all...
    if (section.size() > 0)
    {
        // Invalid index, reset to default (first item)
        if ((size_t)idxbase >= section.size())
            idxbase = 0;
        if ((size_t)idxover >= (section.size()+1))
            idxover = 0;

        // Pick the tileserver config item at index idxbase
        florb::cfg_tileserver ts = section[idxbase].as<florb::cfg_tileserver>(); 
   
        // Try to create a basemap
        try {
            m_wgtmap->basemap(
                ts.name(), 
                ts.url(), 
                ts.zmin(), 
                ts.zmax(), 
                ts.parallel(),
                ts.type()); 
        } catch (std::runtime_error& e) {
            fl_alert("%s", e.what()); 
        }

        if (idxover > 0)
        {
            ts = section[idxover-1].as<florb::cfg_tileserver>();

            // Try to create a basemap
            try {
                m_wgtmap->overlay(
                        ts.name(), 
                        ts.url(), 
                        ts.zmin(), 
                        ts.zmax(), 
                        ts.parallel(),
                        ts.type()); 
            } catch (std::runtime_error& e) {
                fl_alert("%s", e.what()); 
            }

        }
        else
        {
            m_wgtmap->clear_overlay();
        }

        // Set the selected item index
        m_choice_basemap->value(idxbase);
        m_choice_overlay->value(idxover);

        // Focus on the map
        m_wgtmap->take_focus();
    }
}

void dlg_ui::destroy_ex(void)
{
    // Curl cleanup
    curl_global_cleanup();

    // Aaaand we're out
    std::cout << "Goodbye" << std::endl;
}

void dlg_ui::show_ex(int argc, char *argv[])
{
    m_window->show(argc, argv);
}

void dlg_ui::loadtrack_ex()
{
    // Create a file chooser instance
    Fl_File_Chooser fc("/", "*.gpx", Fl_File_Chooser::SINGLE, "Open GPX file");
    fc.preview(0);
    fc.show();

    // Wait for user action
    while(fc.shown())
        Fl::wait();

    // Do nothing on cancel
    if (fc.value() == NULL)
        return;

    // Try to load the track
    try {
        m_wgtmap->gpx_loadtrack(std::string(fc.value()));
    } catch (std::runtime_error& e) {
        fl_alert("%s", e.what());  
    }
}

void dlg_ui::savetrack_ex()
{
    // Create a file chooser instance
    Fl_File_Chooser fc("/", "*.gpx", Fl_File_Chooser::CREATE, "Save GPX file");
    fc.preview(0);
    fc.show();

    // Wait for user action
    while(fc.shown())
        Fl::wait();

    // Do nothing on cancel
    if (fc.value() == NULL)
        return;

    // Load the track
    try {
        m_wgtmap->gpx_savetrack(std::string(fc.value()));
    } catch (std::runtime_error& e) {
        fl_alert("%s", e.what());
    }
}

void dlg_ui::bulkdl_ex()
{
    // Clear all waypoints
    fl_alert("%s", _("Select the desired area on the map now"));
    m_wgtmap->select_area(_("Select download area"));
}

void dlg_ui::eleprofile_ex()
{
    if (!m_dlg_eleprofile)
        m_dlg_eleprofile = new dlg_eleprofile(m_wgtmap);
    
    m_dlg_eleprofile->show();
}

void dlg_ui::cleartrack_ex()
{
    // Clear all waypoints
    m_wgtmap->gpx_cleartrack();
}

void dlg_ui::gotocursor_ex()
{
    // Center over current GPS position
    m_wgtmap->goto_cursor();
}

void dlg_ui::lockcursor_ex()
{
    // Lock / unlock GPS cursor
    bool start = (m_btn_lockcursor->value() == 1) ? true : false;
    m_wgtmap->gpsd_lock(start);
}

void dlg_ui::recordtrack_ex()
{
    // Start / stop recording
    bool start = (m_btn_recordtrack->value() == 1) ? true : false;
    m_wgtmap->gpsd_record(start);
}

void dlg_ui::editselection_ex()
{
    // No waypoints selected
    if (!m_wgtmap->gpx_wpselected())
        return;

    // Show editor dialog for selected waypoints
    if (!m_dlg_editselection)
        m_dlg_editselection = new dlg_editselection(m_wgtmap);
    
    m_dlg_editselection->show();
}

void dlg_ui::deleteselection_ex()
{
    // No waypoints selected
    if (!m_wgtmap->gpx_wpselected())
        return;

    // Delete selected waypoints
    m_wgtmap->gpx_wpdelete();
}

void dlg_ui::showwpmarkers_ex()
{
    // Show / hide waypoint markers
    bool b = (m_menuitem_track_showwpmarkers->value() == 0) ? false : true; 
    m_wgtmap->gpx_showwpmarkers(b);
}

void dlg_ui::garmindl_ex()
{
    // Temp gpx file path
    std::string tmp(florb::utils::appdir() + florb::utils::pathsep() + "tmp.gpx"); 

    // Download track from garmin device to temporary file
    if (!m_dlg_garmindl)
        m_dlg_garmindl = new dlg_garmindl(tmp);

    if (m_dlg_garmindl->show())
    {
        m_wgtmap->gpx_loadtrack(tmp);
        florb::utils::rm(tmp);
    }
}

void dlg_ui::garminul_ex()
{
    //winopen.wait();

    // Temp gpx file path
    std::string name(m_wgtmap->gpx_trackname());
    std::string path(florb::utils::appdir() + florb::utils::pathsep() + "tmp.gpx"); 
    m_wgtmap->gpx_savetrack(path);

    // Show upload dialog
    if (!m_dlg_garminul)
        m_dlg_garminul = new dlg_garminul(path, name);

    m_dlg_garminul->show();

    // Remove temp file
    florb::utils::rm(path);

    //winopen.post();
}

void dlg_ui::settings_ex()
{
    // Create florb::settings dialog
    if (!m_dlg_settings)
        m_dlg_settings = new dlg_settings;

    // If the settings were updated
    if (m_dlg_settings->show())
    {
        florb::settings &s = florb::settings::get_instance();
        
        // Connect / reconnect / disconnect GPSd
        florb::cfg_gpsd cfggpsd = s["gpsd"].as<florb::cfg_gpsd>();
        if (cfggpsd.enabled())
        {
            m_wgtmap->gpsd_connect(cfggpsd.host(), cfggpsd.port());
        }
        else
        {
            m_wgtmap->gpsd_disconnect();
        }

        // Update the list of tileservers
        update_choice_map_ex();
        
        // Update statusbar
        update_statusbar_ex();
    }
}

void dlg_ui::search_ex()
{
    // Create search dialog and show it
    if (!m_dlg_search)
        m_dlg_search = new dlg_search(m_wgtmap);

    m_dlg_search->show();
}

void dlg_ui::about_ex()
{
    // Show about dialog
    if (!m_dlg_about)
        m_dlg_about = new dlg_about;

    m_dlg_about->show();
}

void dlg_ui::cb_btn_loadtrack_ex(Fl_Widget *widget)
{
    loadtrack_ex();
    m_wgtmap->take_focus();
}

void dlg_ui::cb_btn_savetrack_ex(Fl_Widget *widget)
{
    savetrack_ex();
    m_wgtmap->take_focus();
}

void dlg_ui::cb_btn_cleartrack_ex(Fl_Widget *widget)
{
    cleartrack_ex();
    m_wgtmap->take_focus();
}

void dlg_ui::cb_btn_gotocursor_ex(Fl_Widget *widget)
{
    gotocursor_ex();
    m_wgtmap->take_focus();
}

void dlg_ui::cb_btn_lockcursor_ex(Fl_Widget *widget)
{
    if (m_btn_lockcursor->value() != 0)
        m_menuitem_gpsd_lockcursor->set();
    else
        m_menuitem_gpsd_lockcursor->clear();

    lockcursor_ex();
    m_wgtmap->take_focus();
}

void dlg_ui::cb_btn_recordtrack_ex(Fl_Widget *widget)
{
    if (m_btn_recordtrack->value() != 0)
        m_menuitem_gpsd_recordtrack->set();
    else
        m_menuitem_gpsd_recordtrack->clear();

    recordtrack_ex();
    m_wgtmap->take_focus();
}

void dlg_ui::cb_btn_editselection_ex(Fl_Widget *widget)
{
    editselection_ex();
    m_wgtmap->take_focus();
}

void dlg_ui::cb_btn_deleteselection_ex(Fl_Widget *widget)
{
    deleteselection_ex(); 
    m_wgtmap->take_focus();
}

void dlg_ui::cb_choice_basemap_ex(Fl_Widget *widget)
{
    // Get selected item index
    int idx = m_choice_basemap->value();

    // Get all configured tileservers
    std::vector<florb::cfg_tileserver> tileservers(florb::settings::get_instance()["tileservers"].as< std::vector<florb::cfg_tileserver> >());

    // Try to create a basemap using the selected tile server configuration
    try {
        m_wgtmap->basemap(
                tileservers[idx].name(), 
                tileservers[idx].url(), 
                tileservers[idx].zmin(), 
                tileservers[idx].zmax(), 
                tileservers[idx].parallel(),
                tileservers[idx].type());
    } catch (std::runtime_error& e) {
        fl_alert("%s", e.what());        
    }

    // Map focus
    m_wgtmap->take_focus();
}

void dlg_ui::cb_choice_overlay_ex(Fl_Widget *widget)
{
    // Get selected item index
    int idx = m_choice_overlay->value();

    if (idx > 0)
    {
        // Get all configured tileservers
        std::vector<florb::cfg_tileserver> tileservers(florb::settings::get_instance()["tileservers"].as< std::vector<florb::cfg_tileserver> >());

        // Try to create a basemap using the selected tile server configuration
        try {
            m_wgtmap->overlay(
                    tileservers[idx-1].name(), 
                    tileservers[idx-1].url(), 
                    tileservers[idx-1].zmin(), 
                    tileservers[idx-1].zmax(), 
                    tileservers[idx-1].parallel(),
                    tileservers[idx-1].type());
        } catch (std::runtime_error& e) {
            fl_alert("%s", e.what());        
        }
    }
    else
    {
        m_wgtmap->clear_overlay();
    }

    // Map focus
    m_wgtmap->take_focus();
}

void dlg_ui::cb_menu_ex(Fl_Widget *widget)
{
    const Fl_Menu_Item *mit = m_menubar->mvalue();

    // File submenu
    if (mit == m_menuitem_file_quit) {
        hide_ex();

        // All the rest of the application is long gone by now. Nothing left to
        // do but return.
        return;
    }
    else if (mit == m_menuitem_file_opengpx) { 
        loadtrack_ex();
    }
    else if (mit == m_menuitem_file_savegpx) { 
        savetrack_ex();
    } 

    // Edit sumbemnu
    else if (mit == m_menuitem_edit_settings) {
        settings_ex();
    }
    // Edit sumbemnu
    else if (mit == m_menuitem_edit_search) {
        search_ex();
    }
    // Edit sumbemnu
    else if (mit == m_menuitem_edit_bulkdl) {
        bulkdl_ex();
    }

    // Track submenu
    else if (mit == m_menuitem_track_clear) {
        cleartrack_ex();
    }
    else if (mit == m_menuitem_track_editwaypoint) {
        editselection_ex();
    }
    else if (mit == m_menuitem_track_deletewaypoints) {
        deleteselection_ex();
    }
    else if (mit == m_menuitem_track_showwpmarkers) {
        showwpmarkers_ex();
    }
    else if (mit == m_menuitem_track_eleprofile) {
        eleprofile_ex();
    }
    else if (mit == m_menuitem_track_garmindl) {
        garmindl_ex();
    }
    else if (mit == m_menuitem_track_garminul) {
        garminul_ex();
    }
    
    // GPSd submenu
    else if (mit == m_menuitem_gpsd_gotocursor) {
        gotocursor_ex();
    }
    else if (mit == m_menuitem_gpsd_recordtrack) {
        m_btn_recordtrack->value(m_menuitem_gpsd_recordtrack->value());
        recordtrack_ex();
    }
    else if (mit == m_menuitem_gpsd_lockcursor) {
        m_btn_lockcursor->value(m_menuitem_gpsd_lockcursor->value());
        lockcursor_ex();
    }

    // Help submenu
    else if (mit == m_menuitem_help_about) { 
        about_ex();
    }

    m_wgtmap->take_focus();
}

void dlg_ui::cb_close_ex(Fl_Widget *widget)
{
    hide_ex();
}

void dlg_ui::hide_ex()
{
    // Close all non-modal dialogs, we won't be able to get here with any modal
    // dialog still open
    if (m_dlg_search)
        m_dlg_search->hide();

    // Hide the main window
    m_window->hide();

    // Delete all dialogs
    if (m_dlg_garminul)         delete m_dlg_garminul;
    if (m_dlg_garmindl)         delete m_dlg_garmindl;
    if (m_dlg_search)           delete m_dlg_search;
    if (m_dlg_editselection)    delete m_dlg_editselection;
    if (m_dlg_settings)         delete m_dlg_settings;
    if (m_dlg_about)            delete m_dlg_about;
    if (m_dlg_bulkdl)           delete m_dlg_bulkdl;
    if (m_dlg_eleprofile)       delete m_dlg_eleprofile;

    // Delete the main window
    delete(m_window);
}

