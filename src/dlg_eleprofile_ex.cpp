#include "utils.hpp"
#include "fluid/dlg_eleprofile.hpp"

void dlg_eleprofile::create_ex()
{
    register_event_handler<dlg_eleprofile, wgt_eleprofile::event_mouse>(this, &dlg_eleprofile::profile_evt_mouse_ex);
    m_profile->add_event_listener(this);

    // Set the window icon
    florb::utils::set_window_icon(m_window); 
}

void dlg_eleprofile::destroy_ex()
{
    if (m_window)
        delete(m_window);
}

void dlg_eleprofile::show_ex()
{
    std::vector<florb::tracklayer::waypoint> waypoints;
    m_wgtmap->gpx_selection_get(waypoints);
    
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

bool dlg_eleprofile::profile_evt_mouse_ex(const wgt_eleprofile::event_mouse *e)
{
    florb::cfg_units cfgunits = florb::settings::get_instance()["units"].as<florb::cfg_units>();

    std::ostringstream ss; 

    ss.precision(2);
    ss.setf(std::ios::fixed, std::ios::floatfield);

    unit::length dst_trip;
    unit::length dst_ele;
    switch (cfgunits.system_length())
    {
        case (florb::cfg_units::system::NAUTICAL):
            dst_trip = unit::length::SEA_MILE;
            dst_ele = unit::length::FOOT;
            break;
        case (florb::cfg_units::system::IMPERIAL):
            dst_trip = unit::length::ENGLISH_MILE;
            dst_ele = unit::length::FOOT;
            break;
        default:
            dst_trip = unit::length::KM;
            dst_ele = unit::length::M;
            break;
    }

    static std::string strtrip;
    static std::string strele;

    ss.str("");
    ss << _("Trip: ") << unit::convert(unit::length::KM, dst_trip, e->trip()) << " " << unit::sistr(dst_trip);
    strtrip = ss.str();

    ss.str("");
    ss << _("Elevation: ") << unit::convert(unit::length::M, dst_ele, e->ele()) << " " << unit::sistr(dst_ele);
    strele = ss.str();
    
    m_box_trip->label(strtrip.c_str());
    m_box_ele->label(strele.c_str());

    return true;
}

