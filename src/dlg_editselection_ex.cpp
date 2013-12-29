#include <sstream>
#include "fluid/dlg_editselection.hpp"

void dlg_editselection::show_ex()
{
    m_window->show();

    point2d<double> pos(m_mapctrl->selection_pos());
    double ele(m_mapctrl->selection_elevation());

    std::ostringstream os;
    os.precision(6);
    os.setf(std::ios::fixed, std::ios::floatfield);

    os.str("");
    os << pos.x();
    m_txtin_lon->value(os.str().c_str());

    os.str("");
    os << pos.y();
    m_txtin_lat->value(os.str().c_str());

    os.str("");
    os << ele;
    m_txtin_ele->value(os.str().c_str());

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

    is.str(m_txtin_lon->value());
    is >> pos[0];
    is.clear();
    is.str(m_txtin_lat->value());
    is >> pos[1];
    is.clear();
    is.str(m_txtin_ele->value());
    is >> ele;

    m_mapctrl->selection_pos(pos);
    m_mapctrl->selection_elevation(ele);
    
    m_window->hide();
}

