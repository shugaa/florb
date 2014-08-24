#include <FL/fl_draw.H>
#include <FL/x.H>
#include "settings.hpp"
#include "gpxlayer.hpp"
#include "wgt_eleprofile.hpp"

wgt_eleprofile::wgt_eleprofile(int x, int y, int w, int h, const char *label) : 
    Fl_Widget(x, y, w, h, label),
    m_offscreen(w, h)
{
}

wgt_eleprofile::~wgt_eleprofile()
{
}

void wgt_eleprofile::trackpoints(const std::vector<gpxlayer::waypoint>& wpts)
{
    double dst = 0.0;
    m_elemin = 0.0;
    m_elemax = 0.0;

    m_wpts.clear();

    std::vector<gpxlayer::waypoint>::const_iterator it;
    for (it=wpts.begin();it!=wpts.end();++it)
    {
        point2d<double> ptmp;
        ptmp.y((*it).elevation());

        if (ptmp.y() < m_elemin)
            m_elemin = ptmp.y();
        if (ptmp.y() > m_elemax)
            m_elemax = ptmp.y();

        if (it != wpts.begin())
        {
            dst += utils::dist(
                point2d<double>((*it).lon(), (*it).lat()),
                point2d<double>((*(it-1)).lon(), (*(it-1)).lat()));
        }

        ptmp.x(dst);
        m_wpts.push_back(ptmp);
    }
}

void wgt_eleprofile::refresh()
{
    redraw();
}

int wgt_eleprofile::handle(int event) 
{
    int ret = 0;

    switch (event) {
        case FL_MOVE:
            {
                //ret = handle_move(event);
                break;
            }
        case FL_ENTER:
            {
                //ret = handle_enter(event);
                break;
            }
        case FL_LEAVE:
            {
                //ret = handle_leave(event);
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
    m_offscreen.fgcolor(fgfx::color(0xff,0xff,0xff));
    m_offscreen.fillrect(0,0,w(),h());

    double corr = (m_elemin < 0) ? -m_elemin : 0.0;
    double min = m_elemin + corr; 
    double max = m_elemax + corr; 

    double yscale = ((max-min) > 0.0) ? ((double)h())/(max-min) : 0.0;
    double xscale = ((*(m_wpts.end()-1)).x() > 0.0) ? ((double)w())/(*(m_wpts.end()-1)).x() : 0.0;

    m_offscreen.fgcolor(fgfx::color(0xff,0x00,0x00));

    double elast = 0.0;
    std::vector< point2d<double> >::iterator it;
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

