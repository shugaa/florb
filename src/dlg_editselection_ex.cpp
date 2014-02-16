#include <sstream>
#include "fluid/dlg_editselection.hpp"

void dlg_editselection::show_ex()
{
    std::vector<gpxlayer::waypoint> waypoints;
    m_mapctrl->gpx_selection_get(waypoints);

    std::ostringstream os;
    os.precision(6);
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

    m_window->show();

    int r = 0;
    for (;;) {
        Fl_Widget *o = Fl::readqueue();
        if (!o) Fl::wait();
        else if (o == m_btn_ok)     {r=1;break;}
        else if (o == m_btn_cancel) {r=2;break;}
        else if (o == m_window)     {r=3;break;}
    }

    if (r != 1) {
        m_window->hide();
        return;
    }

    // OK, store
    std::istringstream is;

    if (waypoints.size() > 1)
    {
        // Update elevation only, common for every selected
        // waypoint
        std::vector<gpxlayer::waypoint>::iterator it;
        for (it=waypoints.begin();it!=waypoints.end();++it)
        {
            double ele;

            is.str(m_txtin_ele->value());
            is >> ele;
            is.clear();

            (*it).elevation(ele);
        }
    }
    else
    {
        double lat, lon, ele;
        is.str(m_txtin_lon->value());
        is >> lon;
        is.clear();
        is.str(m_txtin_lat->value());
        is >> lat;
        is.clear();
        is.str(m_txtin_ele->value());
        is >> ele;

        waypoints[0].lon(lon);
        waypoints[0].lat(lat);
        waypoints[0].elevation(ele);
    }

    m_mapctrl->gpx_selection_set(waypoints);

    m_window->hide();
}

