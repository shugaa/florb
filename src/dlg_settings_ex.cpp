#include <sstream>
#include <iostream>
#include <FL/Fl_Color_Chooser.H>
#include <Fl/Fl_File_Chooser.H>
#include "settings.hpp"
#include "utils.hpp"
#include "fluid/dlg_settings.hpp"
#include "fluid/dlg_tileserver.hpp"

void dlg_settings::create_ex()
{
    // Set the window icon
    utils::set_window_icon(m_window); 

    m_cfgui = settings::get_instance()["ui"].as<cfg_ui>();
    m_cfggpsd = settings::get_instance()["gpsd"].as<cfg_gpsd>();
    m_cfgtileservers = settings::get_instance()["tileservers"].as< std::vector<cfg_tileserver> >();
    m_cfgcache = settings::get_instance()["cache"].as<cfg_cache>();

    tab_ui_setup_ex();
    tab_gpsd_setup_ex(); 
    tab_tileservers_setup_ex(); 
    tab_cache_setup_ex();
}

void dlg_settings::cb_btn_markercolor_ex(Fl_Widget *widget)
{
    color c = colorchooser_ex(m_cfgui.markercolor());
    
    m_cfgui.markercolor(c);
    m_box_markercolor->color(fl_rgb_color(c.r(), c.g(), c.b()));
    m_box_markercolor->redraw();
}

void dlg_settings::cb_btn_markercolorselected_ex(Fl_Widget *widget)
{
    color c = colorchooser_ex(m_cfgui.markercolorselected());
    
    m_cfgui.markercolorselected(c);
    m_box_markercolorselected->color(fl_rgb_color(c.r(), c.g(), c.b()));
    m_box_markercolorselected->redraw();
}

void dlg_settings::cb_btn_trackcolor_ex(Fl_Widget *widget)
{
    color c = colorchooser_ex(m_cfgui.trackcolor());
    
    m_cfgui.trackcolor(c);
    m_box_trackcolor->color(fl_rgb_color(c.r(), c.g(), c.b()));
    m_box_trackcolor->redraw();
}

void dlg_settings::cb_btn_selectioncolor_ex(Fl_Widget *widget)
{
    color c = colorchooser_ex(m_cfgui.selectioncolor());
    
    m_cfgui.selectioncolor(c);
    m_box_selectioncolor->color(fl_rgb_color(c.r(), c.g(), c.b()));
    m_box_selectioncolor->redraw();
}

void dlg_settings::cb_btn_gpscursorcolor_ex(Fl_Widget *widget)
{
    color c = colorchooser_ex(m_cfgui.gpscursorcolor());
    
    m_cfgui.gpscursorcolor(c);
    m_box_gpscursorcolor->color(fl_rgb_color(c.r(), c.g(), c.b()));
    m_box_gpscursorcolor->redraw();
}

void dlg_settings::cb_inp_trackwidth_ex(Fl_Widget *widget)
{
    if (strlen(m_input_trackwidth->value()) > 0)
    {
        std::istringstream val(m_input_trackwidth->value());
        unsigned int nr;
        val >> nr;
        m_cfgui.tracklinewidth(nr);
    }
}

void dlg_settings::cb_btn_location_ex(Fl_Widget *widget)
{
    // Create a file chooser instance
    Fl_File_Chooser fc(m_cfgcache.location().c_str(), NULL, Fl_File_Chooser::DIRECTORY, _("Select cache directory"));
    fc.preview(0);
    fc.show();

    // Wait for user action
    while(fc.shown())
        Fl::wait();

    // Do nothing on cancel
    if (fc.value() == NULL)
        return;

    // Update location
    m_cfgcache.location(fc.value());
    m_output_location->value(m_cfgcache.location().c_str());
}

void dlg_settings::cb_inp_server_ex(Fl_Widget *widget)
{
    std::string str(m_input_server->value());

    if (str.length() > 0)
    {
        m_cfggpsd.host(str);
    }
}

void dlg_settings::cb_inp_port_ex(Fl_Widget *widget)
{
    std::string str(m_input_port->value());

    if (str.length() > 0)
    {
        m_cfggpsd.port(str);
    }
}

void dlg_settings::cb_chkbtn_enable_ex(Fl_Widget *widget)
{
    if (m_chkbtn_enable->value() > 0)
        m_cfggpsd.enabled(true);
    else
        m_cfggpsd.enabled(false);

    tab_gpsd_setup_ex();
}

void dlg_settings::cb_btn_addserver_ex(Fl_Widget *widget)
{
    cfg_tileserver ts;
    ts.name(_("My tileserver"));
    ts.url("{x}{y}{z}");

    dlg_tileserver dlg(_("Add tile server"), ts);
    if(dlg.show())
    {
        m_cfgtileservers.push_back(dlg.tileserver());
        tab_tileservers_setup_ex();
    }
}

void dlg_settings::cb_btn_delserver_ex(Fl_Widget *widget)
{
    int idx = m_browser_tileservers->value();

    if (idx <= 0)
        return;

    m_cfgtileservers.erase(m_cfgtileservers.begin()+(idx-1));
    m_browser_tileservers->value(0);
    tab_tileservers_setup_ex();
}

void dlg_settings::cb_btn_editserver_ex(Fl_Widget *widget)
{
    int idx = m_browser_tileservers->value();
    if (idx <= 0)
        return;

    dlg_tileserver dlg(_("Edit tile server"), m_cfgtileservers[idx-1]);
    if(dlg.show())
    {
        m_cfgtileservers[idx-1] = dlg.tileserver();
        tab_tileservers_setup_ex();
    }
}

void dlg_settings::tab_ui_setup_ex()
{
    m_box_markercolor->color(fl_rgb_color(m_cfgui.markercolor().r(), m_cfgui.markercolor().g(), m_cfgui.markercolor().b()));
    m_box_markercolorselected->color(fl_rgb_color(m_cfgui.markercolorselected().r(), m_cfgui.markercolorselected().g(), m_cfgui.markercolorselected().b()));
    m_box_trackcolor->color(fl_rgb_color(m_cfgui.trackcolor().r(), m_cfgui.trackcolor().g(), m_cfgui.trackcolor().b()));
    m_box_selectioncolor->color(fl_rgb_color(m_cfgui.selectioncolor().r(), m_cfgui.selectioncolor().g(), m_cfgui.selectioncolor().b()));
    m_box_gpscursorcolor->color(fl_rgb_color(m_cfgui.gpscursorcolor().r(), m_cfgui.gpscursorcolor().g(), m_cfgui.gpscursorcolor().b()));
    m_input_trackwidth->value(static_cast<std::ostringstream*>( &(std::ostringstream() << m_cfgui.tracklinewidth()) )->str().c_str());
}

void dlg_settings::tab_cache_setup_ex()
{
    m_output_location->value(m_cfgcache.location().c_str());
}

void dlg_settings::tab_gpsd_setup_ex()
{
    m_input_server->value(m_cfggpsd.host().c_str());
    m_input_port->value(m_cfggpsd.port().c_str());

    bool en = m_cfggpsd.enabled();

    if (en)
    {
        m_chkbtn_enable->value(1);
        m_box_server->activate();
        m_input_server->activate();
        m_box_port->activate();
        m_input_port->activate();
    }
    else
    {
        m_chkbtn_enable->value(0);
        m_box_server->deactivate();
        m_input_server->deactivate();
        m_box_port->deactivate();
        m_input_port->deactivate(); 
    }
}

void dlg_settings::tab_tileservers_setup_ex()
{
    int idx = m_browser_tileservers->value();

    m_browser_tileservers->clear();

    std::vector<cfg_tileserver>::iterator it;
    for(it=m_cfgtileservers.begin(); it!=m_cfgtileservers.end(); ++it) 
    {
        m_browser_tileservers->add((*it).name().c_str()); 
    }

    if (m_browser_tileservers->size() >= idx)
        m_browser_tileservers->value(idx);
}

color dlg_settings::colorchooser_ex(color c)
{
    double b = c.b() / 255.0;
    double g = c.g() / 255.0;
    double r = c.r() / 255.0;

    int ret = fl_color_chooser( 	
        _("Select color"),
		r, g, b,
		0 
	);

    if (ret == 1)
    {
        c = color(
            (unsigned char)(r*255.0),
            (unsigned char)(g*255.0),
            (unsigned char)(b*255.0));
    }

    return c;
}

bool dlg_settings::show_ex()
{
    m_window->show();
    bool ok = false;
    for (;;) {
        Fl_Widget *o = Fl::readqueue();
        if (!o) Fl::wait();
        else if (o == m_btn_ok)     {ok=true;break;}
        else if (o == m_btn_cancel) {break;}
        else if (o == m_window)     {break;}
    }

    // Save
    if (ok)
    {
        settings::get_instance()["ui"] = m_cfgui;
        settings::get_instance()["gpsd"] = m_cfggpsd;
        settings::get_instance()["tileservers"] = m_cfgtileservers;
        settings::get_instance()["cache"] = m_cfgcache;
    }

    m_window->hide();
    return ok;
}
