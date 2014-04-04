#include <sstream>
#include <Fl/Fl_File_Chooser.H>
#include <curl/curl.h>
#include <locale.h>
#include "gpxlayer.hpp"
#include "mapctrl.hpp"
#include "settings.hpp"
#include "gpsdclient.hpp"
#include "utils.hpp"
#include "fluid/dlg_ui.hpp"
#include "fluid/dlg_editselection.hpp"
#include "fluid/dlg_gpsd.hpp"

static dlg_ui *ui;

int main_ex(int argc, char* argv[])
{
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

    delete ui;

    // Deinit CURL
    curl_global_cleanup();

    return ret;
}

void sighandler_ex(int sig)
{
    ui->hide();
}

bool dlg_ui::mapctrl_evt_notify_ex(const mapctrl::event_notify *e)
{
    std::ostringstream ss; 

    ss << "Zoom: " << m_mapctrl->zoom();
    m_txtout_zoom->value(ss.str().c_str());

    point2d<double> pos = m_mapctrl->mousepos();
    ss.precision(5);
    ss.setf(std::ios::fixed, std::ios::floatfield);

    ss.str("");
    ss << "Lon: " << pos.x() << "°";
    m_txtout_lon->value(ss.str().c_str());

    ss.str("");
    ss << "Lat: " << pos.y() << "°";
    m_txtout_lat->value(ss.str().c_str());

    ss.precision(2);
    ss.setf(std::ios::fixed, std::ios::floatfield);
    ss.str("");
    ss << "Trip: " << m_mapctrl->gpx_trip() << "km";
    m_txtout_trip->value(ss.str().c_str());

    if (m_mapctrl->gpsd_connected())
    {
        m_box_fix->color(FL_YELLOW);

        switch(m_mapctrl->gpsd_mode())
        {
            case (gpsdclient::FIX_NONE):
                m_box_fix->label("?");
                break;
            case (gpsdclient::FIX_2D):
                m_box_fix->color(FL_GREEN);
                m_box_fix->label("2D");
                break;
            case (gpsdclient::FIX_3D):
                m_box_fix->color(FL_GREEN);
                m_box_fix->label("3D");
                break;
        }
    } else
    {
        m_box_fix->color(FL_RED);
        m_box_fix->label("GPS");
    }

    return true;
}

void dlg_ui::create_ex(void)
{
    // Fluid 1.3 does not gettext the menuitems, do it manually here
    // File
    m_menuitem_file->label(_("File"));
    m_menuitem_file_opengpx->label(_("Open GPX"));
    m_menuitem_file_savegpx->label(_("Save GPX as"));
    m_menuitem_file_quit->label(_("Quit"));
    // Edit
    m_menuitem_edit->label(_("Edit"));
    m_menuitem_edit_cleartrack->label(_("Clear track"));
    m_menuitem_edit_editwaypoint->label(_("Edit waypoints"));
    m_menuitem_edit_deletewaypoints->label(_("Delete waypoints"));
    m_menuitem_edit_showwpmarkers->label(_("Show waypoint markers"));
    // GPSs
    m_menuitem_gpsd->label(_("GPSd"));
    m_menuitem_gpsd_settings->label(_("Settings"));
    m_menuitem_gpsd_gotocursor->label(_("Go to cursor"));
    m_menuitem_gpsd_recordtrack->label(_("Record track"));
    m_menuitem_gpsd_lockcursor->label(_("Lock to cursor"));
    // Help
    m_menuitem_help->label(_("Help"));
    m_menuitem_help_about->label(_("About"));

    // Set the window icon
    utils::set_window_icon(m_window);

    // Create UI dialogs
    m_dlg_about = new dlg_about();

    // Register UI callbacks
    m_window->callback(cb_close, this);

    // Populate the Basemap selector
    node section = settings::get_instance()["tileservers"];
    for(node::iterator it=section.begin(); it!=section.end(); ++it) {
        m_choice_basemap->add((*it).as<cfg_tileserver>().name().c_str(), 0, NULL, NULL, 0);
    }
    if (section.size() > 0)
    {
        cfg_tileserver ts0((*section.begin()).as<cfg_tileserver>()); 
        m_mapctrl->basemap(
            ts0.name(), 
            ts0.url(), 
            ts0.zmin(), 
            ts0.zmax(), 
            ts0.parallel(),
            ts0.type()); 
        m_choice_basemap->value(0);
        m_mapctrl->take_focus();
    }

    // Show waypoint markers by default
    m_menuitem_edit_showwpmarkers->set(); 
    m_mapctrl->gpx_showwpmarkers(true);

    // Start listening to mapctrl events
    register_event_handler<dlg_ui, mapctrl::event_notify>(this, &dlg_ui::mapctrl_evt_notify_ex);
    m_mapctrl->add_event_listener(this);
}

void dlg_ui::destroy_ex(void)
{
    // Delete UI dialogs
    delete(m_dlg_about);

    // Delete toplevel window
    delete(m_window);

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
        m_mapctrl->gpx_loadtrack(std::string(fc.value()));
    } catch (...) {}
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
    m_mapctrl->gpx_savetrack(std::string(fc.value()));
}

void dlg_ui::cleartrack_ex()
{
    m_mapctrl->gpx_cleartrack();
}

void dlg_ui::gotocursor_ex()
{
    m_mapctrl->goto_cursor();
}

void dlg_ui::lockcursor_ex()
{
    bool start = (m_btn_lockcursor->value() == 1) ? true : false;
    m_mapctrl->gpsd_lock(start);
}

void dlg_ui::recordtrack_ex()
{
    bool start = (m_btn_recordtrack->value() == 1) ? true : false;
    m_mapctrl->gpsd_record(start);
}

void dlg_ui::editselection_ex()
{
    if (!m_mapctrl->gpx_wpselected())
        return;

    dlg_editselection es(m_mapctrl);
    es.show();
}

void dlg_ui::deleteselection_ex()
{
    if (!m_mapctrl->gpx_wpselected())
        return;

    m_mapctrl->gpx_wpdelete();
}

void dlg_ui::showwpmarkers_ex()
{
    bool b = (m_menuitem_edit_showwpmarkers->value() == 0) ? false : true; 
    m_mapctrl->gpx_showwpmarkers(b);
}

void dlg_ui::gpsdsettings_ex()
{
    dlg_gpsd gd(m_mapctrl);
    gd.show();
}

void dlg_ui::cb_btn_loadtrack_ex(Fl_Widget *widget)
{
    loadtrack_ex();
    m_mapctrl->take_focus();
}

void dlg_ui::cb_btn_savetrack_ex(Fl_Widget *widget)
{
    savetrack_ex();
    m_mapctrl->take_focus();
}

void dlg_ui::cb_btn_cleartrack_ex(Fl_Widget *widget)
{
    cleartrack_ex();
    m_mapctrl->take_focus();
}

void dlg_ui::cb_btn_gotocursor_ex(Fl_Widget *widget)
{
    gotocursor_ex();
    m_mapctrl->take_focus();
}

void dlg_ui::cb_btn_lockcursor_ex(Fl_Widget *widget)
{
    if (m_btn_lockcursor->value() != 0)
        m_menuitem_gpsd_lockcursor->set();
    else
        m_menuitem_gpsd_lockcursor->clear();

    lockcursor_ex();
    m_mapctrl->take_focus();
}

void dlg_ui::cb_btn_recordtrack_ex(Fl_Widget *widget)
{
    if (m_btn_recordtrack->value() != 0)
        m_menuitem_gpsd_recordtrack->set();
    else
        m_menuitem_gpsd_recordtrack->clear();

    recordtrack_ex();
    m_mapctrl->take_focus();
}

void dlg_ui::cb_btn_editselection_ex(Fl_Widget *widget)
{
    editselection_ex();
    m_mapctrl->take_focus();
}

void dlg_ui::cb_btn_gpsd_ex(Fl_Widget *widget)
{
    gpsdsettings_ex(); 
    m_mapctrl->take_focus();
}

void dlg_ui::cb_btn_deleteselection_ex(Fl_Widget *widget)
{
    deleteselection_ex(); 
    m_mapctrl->take_focus();
}

void dlg_ui::cb_choice_basemap_ex(Fl_Widget *widget)
{
    int idx = m_choice_basemap->value();

    std::vector<cfg_tileserver> tileservers(settings::get_instance()["tileservers"].as< std::vector<cfg_tileserver> >());
    m_mapctrl->basemap(
            tileservers[idx].name(), 
            tileservers[idx].url(), 
            tileservers[idx].zmin(), 
            tileservers[idx].zmax(), 
            tileservers[idx].parallel(),
            tileservers[idx].type());

    m_mapctrl->take_focus();
}

void dlg_ui::cb_menu_ex(Fl_Widget *widget)
{
    const Fl_Menu_Item *mit = m_menubar->mvalue();

    // File submenu
    if (mit == m_menuitem_file_quit) {
        m_window->hide();
    }
    else if (mit == m_menuitem_file_opengpx) { 
        loadtrack_ex();
    }
    else if (mit == m_menuitem_file_savegpx) { 
        savetrack_ex();
    } 

    // Edit submenu
    else if (mit == m_menuitem_edit_cleartrack) {
        cleartrack_ex();
    }
    else if (mit == m_menuitem_edit_editwaypoint) {
        editselection_ex();
    }
    else if (mit == m_menuitem_edit_deletewaypoints) {
        deleteselection_ex();
    }
    else if (mit == m_menuitem_edit_showwpmarkers) {
        showwpmarkers_ex();
    }

    // GPSd submenu
    else if (mit == m_menuitem_gpsd_settings) {
        gpsdsettings_ex();
    }
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
        m_dlg_about->show();
    }

    m_mapctrl->take_focus();
}

void dlg_ui::cb_close_ex(Fl_Widget *widget)
{
    m_window->hide();
}

void dlg_ui::hide_ex()
{
    m_window->hide();
}

