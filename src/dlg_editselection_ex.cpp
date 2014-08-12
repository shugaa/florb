#include <FL/fl_ask.H>
#include "fluid/dlg_editselection.hpp"

void dlg_editselection::show_ex()
{
    std::vector<gpxlayer::waypoint> waypoints;
    m_mapctrl->gpx_selection_get(waypoints);

    std::ostringstream os;
    os.precision(8);
    os.setf(std::ios::fixed, std::ios::floatfield);

    os.str("");
    os << waypoints[0].lon();
    m_txtin_lon->value(os.str().c_str());

    os.str("");
    os << waypoints[0].lat();
    m_txtin_lat->value(os.str().c_str());

    os.str("");
    os << waypoints[0].elevation();
    m_txtin_ele->value(os.str().c_str());

    if (waypoints.size() > 1)
    {
        m_txtin_lon->value("-");
        m_txtin_lat->value("-");
        m_txtin_lon->deactivate();
        m_txtin_lat->deactivate();
        m_box_lon->deactivate();
        m_box_lat->deactivate();
    }
    else
    {
        m_txtin_lon->activate();
        m_txtin_lat->activate();
        m_box_lon->activate();
        m_box_lat->activate();
    }

    m_window->show();

    for (;;) {
        Fl_Widget *o = Fl::readqueue();
        if (!o) Fl::wait();
        else if (o == m_btn_ok)     
        {
            if (handle_ok_ex(waypoints))
                break;
        }
        else if (o == m_btn_cancel) {break;}
        else if (o == m_window)     {break;}
    }

    m_window->hide();
}

bool dlg_editselection::handle_ok_ex(std::vector<gpxlayer::waypoint>& waypoints)
{
    if (waypoints.size() > 1)
    {
        // Update elevation only, common for every selected
        // waypoint
        std::vector<gpxlayer::waypoint>::iterator it;
        for (it=waypoints.begin();it!=waypoints.end();++it)
        {
            double ele = 0;
            utils::fromstr(m_txtin_ele->value(), ele);
            (*it).elevation(ele);
        }
    }
    else
    {
        double lat = 0, lon = 0, ele = 0;
        utils::fromstr(m_txtin_lon->value(), lon);
        utils::fromstr(m_txtin_lat->value(), lat);
        utils::fromstr(m_txtin_ele->value(), ele);

        waypoints[0].lon(lon);
        waypoints[0].lat(lat);
        waypoints[0].elevation(ele);
    }

    try {
        m_mapctrl->gpx_selection_set(waypoints);
    } catch (std::out_of_range& e) {
        fl_alert(e.what());
        return false;
    } catch (std::runtime_error& e) {
        fl_alert(e.what());
        return false;
    }

    return true;
}

