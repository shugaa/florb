#include <FL/fl_draw.H>
#include <FL/x.H>
#include "settings.hpp"
#include "tracklayer.hpp"
#include "wgt_eleprofile.hpp"

wgt_eleprofile::wgt_eleprofile(int x, int y, int w, int h, const char *label) : 
    Fl_Widget(x, y, w, h, label),
    m_offscreen(w, h)
{
}

wgt_eleprofile::~wgt_eleprofile()
{
}

void wgt_eleprofile::trackpoints(const std::vector<florb::tracklayer::waypoint>& wpts)
{
    double dst = 0.0;
    m_elemin = 0.0;
    m_elemax = 0.0;

    m_wpts.clear();

    std::vector<florb::tracklayer::waypoint>::const_iterator it;
    for (it=wpts.begin();it!=wpts.end();++it)
    {
        florb::point2d<double> ptmp;
        ptmp.y((*it).elevation());

        if (ptmp.y() < m_elemin)
            m_elemin = ptmp.y();
        if (ptmp.y() > m_elemax)
            m_elemax = ptmp.y();

        if (it != wpts.begin())
        {
            dst += florb::utils::dist(
                florb::point2d<double>((*it).lon(), (*it).lat()),
                florb::point2d<double>((*(it-1)).lon(), (*(it-1)).lat()));
        }

        ptmp.x(dst);
        m_wpts.push_back(ptmp);
    }
}

void wgt_eleprofile::refresh()
{
    redraw();
}

int wgt_eleprofile::handle_move(int event)
{
    if (m_wpts.size() <= 1)
        return 0;

    int posx = Fl::event_x()-x();
    int posy = h() - (Fl::event_y()-y());

    double yscale = ((m_elemax-m_elemin) > 0.0) ? ((double)h())/(m_elemax-m_elemin) : 0.0;
    double xscale = ((*(m_wpts.end()-1)).x() > 0.0) ? ((double)w())/(*(m_wpts.end()-1)).x() : 0.0;

    double trip = (xscale > 0.0) ? posx/xscale : 0.0;
    double ele = (yscale > 0.0) ? posy/yscale : 0.0;

    if (m_elemin < 0)
        ele += m_elemin;

    notify_mouse(trip, ele);
    return 1;
}

int wgt_eleprofile::handle_enter(int event)
{
    fl_cursor(FL_CURSOR_CROSS);
    return 1;
}

int wgt_eleprofile::handle_leave(int event)
{
    fl_cursor(FL_CURSOR_DEFAULT);
    notify_mouse(0.0, 0.0);
    return 1;
}


int wgt_eleprofile::handle(int event) 
{
    int ret = 0;

    switch (event) {
        case FL_MOVE:
            {
                ret = handle_move(event);
                break;
            }
        case FL_ENTER:
            {
                ret = handle_enter(event);
                break;
            }
        case FL_LEAVE:
            {
                ret = handle_leave(event);
                break;
            }
        case FL_PUSH:
            {
                //handle_push(event); 

                // The push event always needs to return 1, otherwise dragging
                // will not work
                ret = 1;
                break;
            }
        case FL_RELEASE:
            {
                //ret = handle_release(event);
                break;
            }
        case FL_DRAG: 
            {
                //ret = handle_drag(event);
                break;
            }
        case FL_MOUSEWHEEL:
            {
                //ret = handle_mousewheel(event);
                break;
            }
        case FL_FOCUS:
            {
                // Focus is accepted
                //ret = 1;
                break;
            }
        case FL_KEYBOARD:
            {
                //ret = handle_keyboard(event);
                break;
            }
    }

    return ret;
}

void wgt_eleprofile::notify_mouse(double trip, double ele)
{
    event_mouse e(trip, ele);
    fire(&e);
}

void wgt_eleprofile::draw() 
{
    // Make sure redraw() has been called previously
    if ((damage() & FL_DAMAGE_ALL) == 0) 
        return;

    // resize the offscreent o match the viewport
    m_offscreen.resize(w(), h());

    // Draw the profile
    draw_profile();

    fl_copy_offscreen(x(), y(), w(), h(), m_offscreen.buf(), 0, 0);
}


void wgt_eleprofile::draw_profile()
{
    m_offscreen.fgcolor(florb::color(0xff,0xff,0xff));
    m_offscreen.fillrect(0,0,w(),h());

    if (m_wpts.size() <= 1)
        return;

    double corr = (m_elemin < 0) ? -m_elemin : 0.0;
    double min = m_elemin + corr; 
    double max = m_elemax + corr; 

    double yscale = ((max-min) > 0.0) ? ((double)h())/(max-min) : 0.0;
    double xscale = ((*(m_wpts.end()-1)).x() > 0.0) ? ((double)w())/(*(m_wpts.end()-1)).x() : 0.0;

    m_offscreen.fgcolor(florb::color(0xff,0x00,0x00));

    double elast = 0.0;
    std::vector< florb::point2d<double> >::iterator it;
    for (it=m_wpts.begin();it!=m_wpts.end();++it)
    {
        double ecurrent = (*it).y() + corr;

        if (it == m_wpts.begin())
        {
            elast = (*it).y() + corr;
            continue;
        }
        
        m_offscreen.line(
                (int)((*(it-1)).x()*xscale), h()-(int)(elast*yscale),
                (int)((*it).x()*xscale), h()-(int)(ecurrent*yscale),
                1);
        
        elast = ecurrent;
    }
}

