#include "utils.hpp"
#include "fluid/dlg_eleprofile.hpp"

void dlg_eleprofile::create_ex()
{
    // Set the window icon
    utils::set_window_icon(m_window); 
}

void dlg_eleprofile::destroy_ex()
{
    if (m_window)
        delete(m_window);
}

void dlg_eleprofile::show_ex()
{
    std::vector<gpxlayer::waypoint> waypoints;
    m_mapctrl->gpx_selection_get(waypoints);
    
    m_profile->trackpoints(waypoints);

    // Show the window
    m_window->show();

    for (;;) {
        Fl_Widget *o = Fl::readqueue();
        if (!o) Fl::wait();
        else if (o == m_btn_ok)     {break;}
        else if (o == m_window)     {break;}
    }

    m_window->hide();
}

