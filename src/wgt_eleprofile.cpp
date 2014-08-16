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
    m_wpts = wpts;
}

void wgt_eleprofile::refresh()
{
    // Quote from the doc: The public method Fl_Widget::redraw() simply does
    // Fl_Widget::damage(FL_DAMAGE_ALL)
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

    double min = 0.0, max = 0.0, corr = 0.0; 

    std::vector<gpxlayer::waypoint>::iterator it;
    for (it=m_wpts.begin();it!=m_wpts.end();++it)
    {
        double e = (*it).elevation();

        if (e < min)
            min = e;
        if (e > max)
            max = e;
    }

    if (min < 0)
    {
        corr -= min;
        min = 0.0;
        max += corr;
    }

    double yscale = ((double)h())/(max-min);
    double xscale = ((double)w())/((double)m_wpts.size());

    m_offscreen.fgcolor(fgfx::color(0xff,0x00,0x00));

    double x = 0;
    for (it=m_wpts.begin();it!=m_wpts.end();++it)
    {
        static double elast = (*it).elevation() + corr;
        double ecurrent = (*it).elevation() + corr;

        if (it == m_wpts.begin())
            continue;
        
        m_offscreen.line(
                (int)((x-1.0)*xscale), h()-(int)(elast*yscale),
                (int)((x)*xscale), h()-(int)(ecurrent*yscale),
                1);
        
        elast = ecurrent;
        x+=1.0;
    }
}

