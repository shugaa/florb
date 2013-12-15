#include <sstream>
#include <Fl/Fl_File_Chooser.H>
#include "gpxlayer.hpp"
#include "mapctrl.hpp"
#include "settings.hpp"
#include "fluid/dlg_ui.hpp"

void dlg_ui::mapctrl_notify()
{
    std::ostringstream ss; 

    ss << "Zoom: " << m_mapctrl->zoom();
    m_txtout_zoom->value(ss.str().c_str());

    point2d<double> pos = m_mapctrl->mousegps();
    ss.precision(5);
    ss.setf(std::ios::fixed, std::ios::floatfield);

    ss.str("");
    ss << "Lon: " << pos.x() << "°";
    m_txtout_lon->value(ss.str().c_str());

    ss.str("");
    ss << "Lat: " << pos.y() << "°";
    m_txtout_lat->value(ss.str().c_str());
}

void dlg_ui::create_ex(void)
{
    // Create UI dialogs
    m_dlg_about = new orb_dlg_about();
    m_dlg_layers = new orb_dlg_layers();
    m_dlg_settings = new orb_dlg_settings();
    m_dlg_elpro = new orb_dlg_elpro();
    m_dlg_servers = new dlg_servers();

    // Register UI callbacks
    m_window->callback(cb_close, this);
    m_menu_help_about->callback(cb_menu, this);
    m_menu_file_close->callback(cb_menu, this);
    m_menu_edit_layers->callback(cb_menu, this);
    m_menu_edit_servers->callback(cb_menu, this);
    m_menu_edit_settings->callback(cb_menu, this);
    m_menu_view_elevationprofile->callback(cb_menu, this);

    // Populate the Basemap selector
    node section = settings::get_instance()["tileservers"];
    for(node::iterator it=section.begin(); it!=section.end(); ++it) {
        m_choice_basemap->add((*it).as<cfg_tileserver>().name.c_str(), 0, NULL, NULL, 0);
    }
    if (section.size() > 0)
    {
        cfg_tileserver ts0((*section.begin()).as<cfg_tileserver>()); 
        m_mapctrl->basemap(
            ts0.name, 
            ts0.url, 
            ts0.zmin, 
            ts0.zmax, 
            ts0.parallel,
            ts0.type); 
        m_choice_basemap->value(0);
        m_mapctrl->take_focus();
    }

    m_mapctrl->addobserver(*this);
}

void dlg_ui::destroy_ex(void)
{
    // Delete UI dialogs
    delete(m_dlg_about);
    delete(m_dlg_layers);
    delete(m_dlg_settings);
    delete(m_dlg_elpro);
    delete(m_dlg_servers);

    // Delete toplevel window
    delete(m_window);

    // Aaaand we're out
    std::cout << "Goodbye" << std::endl;
}

void dlg_ui::show_ex(int argc, char *argv[])
{
    m_window->show(argc, argv);
}

void dlg_ui::cb_btn_loadtrack_ex(Fl_Widget *widget)
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

    // Try to create a new GPX layer from the file
    //layer *l = new gpxlayer(std::string(fc.value()));

    // Display the new layer
    //m_mapctrl->push_layer(l);
}

void dlg_ui::cb_choice_basemap_ex(Fl_Widget *widget)
{
    int idx = m_choice_basemap->value();

    std::vector<cfg_tileserver> tileservers(settings::get_instance()["tileservers"].as< std::vector<cfg_tileserver> >());
    m_mapctrl->basemap(
            tileservers[idx].name, 
            tileservers[idx].url, 
            tileservers[idx].zmin, 
            tileservers[idx].zmax, 
            tileservers[idx].parallel,
            tileservers[idx].type);

    m_mapctrl->take_focus();
}

void dlg_ui::cb_menu_ex(Fl_Widget *widget)
{
    char picked[80];
    m_menubar->item_pathname(picked, sizeof(picked)-1);

    if (strcmp(picked, "File/Quit") == 0) {
        m_window->hide();
    }
    else if (strcmp(picked, "Help/About") == 0) { 
        m_dlg_about->show();
    }
    else if (strcmp(picked, "Edit/Layers") == 0) {
        // Show the layers editor dialog
        int rc = m_dlg_layers->show();
        if (rc != 0)
            return;
    }
    else if (strcmp(picked, "Edit/Settings") == 0) {
        m_dlg_settings->show();
    }
    else if (strcmp(picked, "Edit/Tileservers") == 0) {
        m_dlg_servers->show();
    }
    else if (strcmp(picked, "View/Elevation profile") == 0) {
        m_dlg_elpro->show();
    }

    m_mapctrl->refresh();
}

void dlg_ui::cb_close_ex(Fl_Widget *widget)
{
    m_window->hide();
}
