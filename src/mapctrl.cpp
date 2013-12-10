#include <cmath>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <FL/fl_draw.H>
#include <FL/x.H>

#include "settings.hpp"
#include "osmlayer.hpp"
#include "gpxlayer.hpp"
#include "gpsdlayer.hpp"
#include "mapctrl.hpp"
#include "utils.hpp"


mapctrl::mapctrl(int x, int y, int w, int h, const char *label) : 
    Fl_Widget(x, y, w, h, label),
    m_basemap(NULL),
    m_mousepos(0, 0),
    m_viewport((unsigned long)w, (unsigned long)h),
    m_offscreen(500,500)
{
}

mapctrl::~mapctrl()
{
    for (std::vector<layer*>::iterator iter=m_layers.begin();iter!=m_layers.end();++iter)
        delete (*iter);
}

void mapctrl::layer_notify()
{
    redraw();
}

int mapctrl::zoom()
{
    // Return current zoomlevel
    return m_viewport.z();
}

void mapctrl::zoom(unsigned int z)
{
    // Set tne new zoomlevel
    m_viewport.z(z, m_viewport.w()/2, m_viewport.h()/2);
   
    // Issue a WAKEUP message to the drawing thread.
    refresh();
}

void mapctrl::push_layer(layer* l)
{
    if (m_layers.size() > 0)
    {
        delete m_layers.back();
        m_layers.pop_back();
    }

    l->addobserver(*this);
    m_layers.push_back(l);
}

void mapctrl::basemap(
                const std::string& name, 
                const std::string& url, 
                int zmin, 
                int zmax, 
                int parallel)
{
    // Save a reference to the original basemap layer. This layer is not
    // removed before the new basemap layer is created, so the cache won't be
    // closed and reopened in the process.
    layer *tmp = m_basemap;
    m_basemap = NULL;

    // Create a new basemap layer
    m_basemap = new osmlayer(url, parallel);
    m_basemap->addobserver(*this);

    // Destroy the original basemap layer 
    if (tmp)
    {
        delete tmp;
    }

    // Redraw
    refresh();
}

point<double> mapctrl::mousegps()
{
    // Calculate the mouse's current pixel position on the active viewport
    unsigned long dpx = 0, dpy = 0;
    if ((unsigned long)w() > m_viewport.w())
        dpx = ((unsigned long)w() - m_viewport.w())/2;
    if ((unsigned long)h() > m_viewport.h())
        dpy = ((unsigned long)h() - m_viewport.h())/2;

    unsigned long px = m_mousepos.get_x();
    unsigned long py = m_mousepos.get_y();

    if (dpx > px)
        px = 0;
    else
        px -= dpx;
    if (dpy > py)
        py = 0;
    else
        py -= dpy;

    if (px >= m_viewport.w())
        px = m_viewport.w()-1;
    if (py >= m_viewport.h())
        py = m_viewport.h()-1;

    // Get the GPS coordinates for the current mouse position
    point<double> gps;
    utils::px2gps(
            m_viewport.z(), 
            point<unsigned int>(m_viewport.x()+px, m_viewport.y()+py), 
            gps);

    return gps;
}

void mapctrl::refresh()
{
    redraw();
}

int mapctrl::handle(int event) 
{
    switch (event) {
        case FL_MOVE:
            // Save the current mouse position
            m_mousepos.set_x(Fl::event_x()-x());
            m_mousepos.set_y(Fl::event_y()-y());
            refresh();
            return 1;
        case FL_ENTER:
            fl_cursor(FL_CURSOR_HAND);
            return 1;
        case FL_LEAVE:
            fl_cursor(FL_CURSOR_DEFAULT);
            return 1;
        case FL_PUSH:
            if (Fl::event_button() == FL_LEFT_MOUSE);
            return 1;
        case FL_RELEASE: 
            return 1;
        case FL_DRAG: 
            {
                if (!Fl::event_inside(this))
                    break;

                // Calculate the delta with the last mouse position and save the
                // current mouse position
                int dx = m_mousepos.get_x() - (Fl::event_x()-x());
                int dy = m_mousepos.get_y() - (Fl::event_y()-y());
                m_mousepos.set_x(Fl::event_x()-x());
                m_mousepos.set_y(Fl::event_y()-y());

                // Move the viewport accordingly and redraw
                m_viewport.move((long)dx, (long)dy); 
                refresh();

                return 1;
            }
        case FL_MOUSEWHEEL:
            if (!Fl::event_inside(this))
                break;

            // Prevent integer underflow
            if ((Fl::event_dy() > 0) && (m_viewport.z() == 0))
                return 1;

            // The image of the viewport might be smaller then our current
            // client area. We need to take this delta into account.
            int dpx = 0, dpy = 0;
            if (w() > (int)m_viewport.w())
                dpx = (w() - (int)m_viewport.w())/2;
            if (h() > (int)m_viewport.h())
                dpy = (h() - (int)m_viewport.h())/2;

            int px = 0, py = 0;
            if ((Fl::event_x() - x()) > dpx)
                px = Fl::event_x() - x() - dpx;
            if ((Fl::event_y() - y()) > dpy)
                py = Fl::event_y() - y() - dpy;

            // Zoom the viewport with (px,py) as origin
            m_viewport.z(m_viewport.z()-Fl::event_dy(), px, py);
            refresh();
            return 1;
    }

    return Fl_Widget::handle(event);
}

void mapctrl::draw() 
{
    // Resize the viewport before drawing
    m_viewport.w((long)w());
    m_viewport.h((long)h());

    if ((damage() & FL_DAMAGE_ALL) == 0) 
        return;

    // Fill the area which the viewport does not cover
    fl_rectf(x(), y(), w(), h(), 80, 80, 80);

    // Create ancanvas drawing buffer and send all subsequent commands there
    m_offscreen.resize(m_viewport.w(), m_viewport.h());
    fl_begin_offscreen(m_offscreen.buf());

    // Background-fill thecanvas buffer (tiles might be missing)
    fl_rectf(0, 0, m_viewport.w(), m_viewport.h(), 80, 80, 80);

    // Draw the basemap
    if (m_basemap)
    {
        m_basemap->draw(m_viewport, m_offscreen);
    }

    // Draw all the layers
    for (std::vector<layer*>::iterator iter=m_layers.begin();iter!=m_layers.end();++iter)
        (*iter)->draw(m_viewport, m_offscreen);

    // Blit the generated viewport bitmap onto the widget (centered)
    int dpx = 0, dpy = 0;
    if (w() > (int)m_viewport.w())
        dpx = (w() - (int)m_viewport.w())/2;
    if (h() > (int)m_viewport.h())
        dpy = (h() - (int)m_viewport.h())/2;

    fl_end_offscreen();
    fl_copy_offscreen(x()+dpx, y()+dpy, m_viewport.w(), m_viewport.h(), m_offscreen.buf(), 0, 0);
}

void mapctrl::resize(int x, int y, int w, int h)
{
    Fl_Widget::resize(x, y, w, h);
}

