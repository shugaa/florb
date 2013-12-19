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
    m_gpxlayer(NULL),
    m_gpsdlayer(NULL),
    m_mousepos(0, 0),
    m_viewport((unsigned long)w, (unsigned long)h),
    m_offscreen(500,500)
{
    m_gpxlayer = new gpxlayer();
    m_gpxlayer->addobserver(*this);

    m_gpsdlayer = new gpsdlayer();
    m_gpsdlayer->addobserver(*this);
}

mapctrl::~mapctrl()
{
    if (m_basemap)
        delete m_basemap;

    if (m_gpxlayer)
        delete m_gpxlayer;

    if (m_gpsdlayer)
        delete m_gpsdlayer;
}

void mapctrl::layer_notify()
{
    // Quote from the doc The public method Fl_Widget::redraw() simply does
    // Fl_Widget::damage(FL_DAMAGE_ALL)
    refresh();
}

unsigned int mapctrl::zoom()
{
    // Return current zoomlevel
    return m_viewport.z();
}

void mapctrl::zoom(unsigned int z)
{
    // Set tne new zoomlevel
    m_viewport.z(z, m_viewport.w()/2, m_viewport.h()/2);
   
    // Request a redraw
    refresh();
}

void mapctrl::basemap(
                const std::string& name, 
                const std::string& url, 
                unsigned int zmin, 
                unsigned int zmax, 
                unsigned int parallel,
                int imgtype)
{
    // Save a reference to the original basemap layer. This layer is not
    // removed before the new basemap layer is created, so the cache won't be
    // closed and reopened in the process.
    layer *tmp = m_basemap;
    m_basemap = NULL;

    // Create a new basemap layer
    m_basemap = new osmlayer(name, url, zmin, zmax, parallel, imgtype);
    m_basemap->addobserver(*this);

    // Destroy the original basemap layer 
    if (tmp)
    {
        delete tmp;
    }

    // Redraw
    refresh();
}

point2d<double> mapctrl::mousegps()
{
    // The currently active viewport might be smaller than the current widget
    // size. Calculate the delta first
    unsigned long dpx = 0, dpy = 0;
    if ((unsigned long)w() > m_viewport.w())
        dpx = ((unsigned long)w() - m_viewport.w())/2;
    if ((unsigned long)h() > m_viewport.h())
        dpy = ((unsigned long)h() - m_viewport.h())/2;

    // Get the current mouse position over the widget
    unsigned long px = m_mousepos.x();
    unsigned long py = m_mousepos.y();

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
    point2d<double> gps;
    utils::px2gps(
            m_viewport.z(), 
            point2d<unsigned int>(m_viewport.x()+px, m_viewport.y()+py), 
            gps);

    return gps;
}

void mapctrl::refresh()
{
    // Quote from the doc: The public method Fl_Widget::redraw() simply does
    // Fl_Widget::damage(FL_DAMAGE_ALL)
    redraw();
}


void mapctrl::addobserver(mapctrl_observer &o)
{
    m_observers.insert(&o);
}

void mapctrl::removeobserver(mapctrl_observer &o)
{
    m_observers.erase(&o);
}

void mapctrl::notify_observers()
{
    std::set<mapctrl_observer*>::iterator it;
    for (it = m_observers.begin(); it != m_observers.end(); it++)
        (*it)->mapctrl_notify();
}

point2d<int> mapctrl::vp_relative(const point2d<int>& pos)
{
    // Convert to widget-relative coordinates first
    point2d<int> ret(pos.x()-x(), pos.y()-y());

    // Calculate the widget<->viewport delta and get the viewport-relative
    // return coordinates
    if (w() > (int)m_viewport.w())
        ret[0] -= (w() - (int)m_viewport.w())/2;
    if (h() > (int)m_viewport.h())
        ret[1] -= (h() - (int)m_viewport.h())/2;

    return ret;
}

bool mapctrl::vp_inside(const point2d<int>& pos)
{
    point2d<int> vprel(vp_relative(pos));

    if (vprel.x() < 0)
        return false;
    if (vprel.x() >= (int)m_viewport.w())
        return false;
    if (vprel.y() < 0)
        return false;
    if (vprel.y() >= (int)m_viewport.h())
        return false;

    return true;
}

int mapctrl::handle(int event) 
{
    switch (event) {
        case FL_MOVE:
            // Save the current mouse position
            m_mousepos.x(Fl::event_x()-x());
            m_mousepos.y(Fl::event_y()-y());
            notify_observers();
            return 1;
        case FL_ENTER:
            fl_cursor(FL_CURSOR_HAND);
            return 1;
        case FL_LEAVE:
            fl_cursor(FL_CURSOR_DEFAULT);
            return 1;
        case FL_PUSH:
            {
                take_focus();

                // Mouse event for the layers
                int button = layer_mouseevent::BUTTON_MIDDLE;
                if (Fl::event_button() == FL_LEFT_MOUSE)
                    button = layer_mouseevent::BUTTON_LEFT;
                else if (Fl::event_button() == FL_RIGHT_MOUSE)
                    button = layer_mouseevent::BUTTON_RIGHT;

                layer_mouseevent me(
                        m_viewport, 
                        layer_mouseevent::ACTION_PRESS,
                        button, 
                        vp_relative(point2d<int>(Fl::event_x(), Fl::event_y())));
                
                m_gpxlayer->handle(&me);

                // The push event always needs to return 1, otherwise dragging
                // will not work
                return 1;
            }
        case FL_RELEASE:
            {
                // Mouse event for the layers
                int button = layer_mouseevent::BUTTON_MIDDLE;
                if (Fl::event_button() == FL_LEFT_MOUSE)
                    button = layer_mouseevent::BUTTON_LEFT;
                else if (Fl::event_button() == FL_RIGHT_MOUSE)
                    button = layer_mouseevent::BUTTON_RIGHT;

                layer_mouseevent me(
                        m_viewport, 
                        layer_mouseevent::ACTION_RELEASE,
                        button, 
                        vp_relative(point2d<int>(Fl::event_x(), Fl::event_y())));
                
                return (m_gpxlayer->handle(&me) ? 1 : 0);
            }
        case FL_DRAG: 
            {
                //if (!Fl::event_inside(this))
                //    break;

                // Mouse event for the layers
                int button = layer_mouseevent::BUTTON_MIDDLE;
                if (Fl::event_button() == FL_LEFT_MOUSE)
                    button = layer_mouseevent::BUTTON_LEFT;
                else if (Fl::event_button() == FL_RIGHT_MOUSE)
                    button = layer_mouseevent::BUTTON_RIGHT;

                layer_mouseevent me(
                        m_viewport, 
                        layer_mouseevent::ACTION_DRAG,
                        button, 
                        vp_relative(point2d<int>(Fl::event_x(), Fl::event_y())));
                
                int ret = (m_gpxlayer->handle(&me) ? 1 : 0);
 
                if (Fl::event_state(FL_BUTTON3) != 0)
                {
                    // Calculate the delta with the last mouse position and save the
                    // current mouse position
                    int dx = m_mousepos.x() - (Fl::event_x()-x());
                    int dy = m_mousepos.y() - (Fl::event_y()-y());
                
                    m_mousepos.x(Fl::event_x()-x());
                    m_mousepos.y(Fl::event_y()-y());

                    // Move the viewport accordingly and redraw
                    m_viewport.move((long)dx, (long)dy); 
                    refresh();

                    ret = 1;
                }

                return ret;
            }
        case FL_MOUSEWHEEL:
            {
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

                notify_observers();
                return 1;
            }
        case FL_FOCUS:
            return 1;
        case FL_KEYBOARD:
            {
                if (!Fl::event_key(FL_Delete))
                    return 0;      

                layer_keyevent ke(
                        layer_keyevent::ACTION_RELEASE, 
                        layer_keyevent::KEY_DEL);
                
                return (m_gpxlayer->handle(&ke) ? 1 : 0);
            }
    }

    // Event unhandled
    return 0;
}

void mapctrl::draw() 
{
    // Make sure redraw() has been called previously
    if ((damage() & FL_DAMAGE_ALL) == 0) 
        return;

    // Resize the viewport to the current widget size before drawing
    m_viewport.w((unsigned long)w());
    m_viewport.h((unsigned long)h());

    // Fill the canvas in case it is not entirely covered by the viewport image
    // to be generated.
    fl_rectf(x(), y(), w(), h(), 80, 80, 80);

    // Create ancanvas drawing buffer and send all subsequent commands there
    m_offscreen.resize(m_viewport.w(), m_viewport.h());
    fl_begin_offscreen(m_offscreen.buf());

    // Background-fill the canvas (there might be no basemap selected)
    fl_rectf(0, 0, m_viewport.w(), m_viewport.h(), 80, 80, 80);

    // Draw the basemap
    if (m_basemap)
        m_basemap->draw(m_viewport, m_offscreen);

    // Draw the gpx layer
    if (m_gpxlayer)
        m_gpxlayer->draw(m_viewport, m_offscreen);

    // Draw the gpsd layer
    if (m_gpsdlayer)
        m_gpsdlayer->draw(m_viewport, m_offscreen);

    // Blit the generated viewport canvas onto the widget (centered)
    int dpx = 0, dpy = 0;
    if (w() > (int)m_viewport.w())
        dpx = (w() - (int)m_viewport.w())/2;
    if (h() > (int)m_viewport.h())
        dpy = (h() - (int)m_viewport.h())/2;

    fl_end_offscreen();
    fl_copy_offscreen(x()+dpx, y()+dpy, m_viewport.w(), m_viewport.h(), m_offscreen.buf(), 0, 0);
}
