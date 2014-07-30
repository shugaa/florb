#include <cmath>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <FL/fl_draw.H>
#include <FL/x.H>
#include "settings.hpp"
#include "mapctrl.hpp"
#include "utils.hpp"

mapctrl::mapctrl(int x, int y, int w, int h, const char *label) : 
    Fl_Widget(x, y, w, h, label),
    m_basemap(NULL),
    m_gpxlayer(NULL),
    m_gpsdlayer(NULL),
    m_mousepos(0, 0),
    m_viewport((unsigned long)w, (unsigned long)h),
    m_offscreen(w,h),
    m_lockcursor(false),
    m_recordtrack(false)
{
    // Register event handlers for layer events
    register_event_handler<mapctrl, gpsdlayer::event_status>(this, &mapctrl::gpsd_evt_status);
    register_event_handler<mapctrl, gpsdlayer::event_motion>(this, &mapctrl::gpsd_evt_motion);
    register_event_handler<mapctrl, osmlayer::event_notify>(this, &mapctrl::osm_evt_notify);
    register_event_handler<mapctrl, gpxlayer::event_notify>(this, &mapctrl::gpx_evt_notify);
    register_event_handler<mapctrl, markerlayer::event_notify>(this, &mapctrl::marker_evt_notify);

    // Add a GPX layer
    m_gpxlayer = new gpxlayer();
    if (!m_gpxlayer)
        throw 0;

    m_gpxlayer->add_event_listener(this);
    add_event_listener(m_gpxlayer);

    // Add a marker layer
    m_markerlayer = new markerlayer();
    if (!m_markerlayer)
        throw 0;

    m_markerlayer->add_event_listener(this);
    add_event_listener(m_markerlayer);

    // Add a gpsdlayer if enabled
    cfg_gpsd cfggpsd = settings::get_instance()["gpsd"].as<cfg_gpsd>();
    if (cfggpsd.enabled())
        gpsd_connect(cfggpsd.host(), cfggpsd.port());  

    // Restore previous viewport
    cfg_viewport cfgvp = settings::get_instance()["viewport"].as<cfg_viewport>();

    // Set previous zoom level
    if (cfgvp.z() > m_viewport.z())
        m_viewport.z(cfgvp.z(), m_viewport.w()/2, m_viewport.h()/2);

    // set previous position
    goto_pos(point2d<double>(cfgvp.lon(), cfgvp.lat()));
}

mapctrl::~mapctrl()
{
    // Save viewport configuration
    cfg_viewport cfgvp = settings::get_instance()["viewport"].as<cfg_viewport>();
    cfgvp.z(m_viewport.z());
    point2d<double> pos = utils::px2wsg84(
            m_viewport.z(), 
            point2d<unsigned long>(m_viewport.x()+(m_viewport.w()/2), m_viewport.y()+(m_viewport.h()/2)));

    cfgvp.lon(pos.x());
    cfgvp.lat(pos.y());
    settings::get_instance()["viewport"] = cfgvp;

    // Delete all layers if active
    if (m_basemap)
        delete m_basemap;

    if (m_gpxlayer)
        delete m_gpxlayer;

    if (m_markerlayer)
        delete m_markerlayer;

    if (m_gpsdlayer)
        delete m_gpsdlayer;
}

void mapctrl::goto_pos(const point2d<double> &pwsg84)
{
    // set previous position
    point2d<unsigned long> ppx(utils::wsg842px(zoom(), pwsg84));
    m_viewport.x(ppx.x() - (m_viewport.w()/2));
    m_viewport.y(ppx.y() - (m_viewport.h()/2));

    refresh();
}

void mapctrl::marker_add(const point2d<double> &pmerc, size_t id)
{
    if (!m_markerlayer)
        throw std::runtime_error(_("Marker error"));

    m_markerlayer->add(pmerc, id);
}

size_t mapctrl::marker_add(const point2d<double> &pmerc)
{
    if (!m_markerlayer)
        throw std::runtime_error(_("Marker error"));

    return m_markerlayer->add(pmerc);
}

void mapctrl::marker_remove(size_t id)
{
    if (!m_markerlayer)
        throw std::runtime_error(_("Marker error"));

    m_markerlayer->remove(id);
}

void mapctrl::gpx_loadtrack(const std::string& path)
{
    if (!m_gpxlayer)
        throw 0;

    m_gpxlayer->load_track(path);
}

void mapctrl::gpx_savetrack(const std::string& path)
{
    if (!m_gpxlayer)
        throw 0;

    m_gpxlayer->save_track(path);
}

void mapctrl::gpx_cleartrack()
{
    if (!m_gpxlayer)
        throw 0;

    m_gpxlayer->clear_track();
}

bool mapctrl::gpx_wpselected()
{
    if (!m_gpxlayer)
        throw 0;

    return (m_gpxlayer->selected() > 0);
}

void mapctrl::gpx_selection_get(std::vector<gpxlayer::waypoint>& waypoints)
{
    if (!m_gpxlayer)
        throw 0;

    m_gpxlayer->selection_get(waypoints);
}
        
void mapctrl::gpx_selection_set(const std::vector<gpxlayer::waypoint>& waypoints)
{
    if (!m_gpxlayer)
        throw 0;

    m_gpxlayer->selection_set(waypoints);
    refresh();
}

void mapctrl::gpx_wpdelete()
{
    if (!m_gpxlayer)
        throw 0;

    m_gpxlayer->selection_delete();    
}

double mapctrl::gpx_trip()
{
    if (!m_gpxlayer)
        throw 0;

    return m_gpxlayer->trip();
}

void mapctrl::gpx_showwpmarkers(bool s)
{
    if (!m_gpxlayer)
        throw 0;

    m_gpxlayer->showwpmarkers(s);
}

std::string mapctrl::gpx_trackname()
{
    if (!m_gpxlayer)
        throw 0;

    return m_gpxlayer->name();
}

bool mapctrl::gpsd_connected()
{
    if (!m_gpsdlayer)
        return false;

    return m_gpsdlayer->connected();
}

void mapctrl::gpsd_connect(const std::string& host, const std::string& port)
{
    if (m_gpsdlayer)
        gpsd_disconnect();

    m_gpsdlayer = new gpsdlayer(host, port);
    m_gpsdlayer->add_event_listener(this);
}

void mapctrl::gpsd_disconnect()
{
    // Not connected in the first place
    if (!m_gpsdlayer)
        return;

    // Disconnect
    remove_event_listener(m_gpsdlayer);
    delete m_gpsdlayer;
    m_gpsdlayer = NULL;
    
    // Refresh display and notify
    refresh();
    event_notify e;
    fire(&e);
}

void mapctrl::gpsd_record(bool start)
{
    m_recordtrack = start;
}

void mapctrl::gpsd_lock(bool start)
{
    m_lockcursor = start;
    if (m_lockcursor)
        goto_cursor();
}

int mapctrl::gpsd_mode()
{
    if (!m_gpsdlayer)
        throw 0;

    return m_gpsdlayer->mode();
}

void mapctrl::basemap(
                const std::string& name, 
                const std::string& url, 
                unsigned int zmin, 
                unsigned int zmax, 
                unsigned int parallel,
                int imgtype)
{
    osmlayer *lold = m_basemap;
    m_basemap = NULL;

    // Destroy the orig
    if (lold)
    {
        remove_event_listener(lold);
        delete lold;
    }

    // Create a new basemap layer
    try {
        m_basemap = new osmlayer(name, url, zmin, zmax, parallel, imgtype);
    } catch (std::runtime_error& e) {
        m_basemap = NULL;
        throw e;
    }

    m_basemap->add_event_listener(this);
    add_event_listener(m_basemap);

    // Redraw
    refresh();
}

void mapctrl::overlay(
                const std::string& name, 
                const std::string& url, 
                unsigned int zmin, 
                unsigned int zmax, 
                unsigned int parallel,
                int imgtype)
{
    clear_overlay();

    // Create a new overlay layer
    try {
        m_overlay = new osmlayer(name, url, zmin, zmax, parallel, imgtype);
    } catch (std::runtime_error& e) {
        m_overlay = NULL;
        throw e;
    }

    m_overlay->add_event_listener(this);
    add_event_listener(m_overlay);

    // Redraw
    refresh();
}

void mapctrl::clear_overlay()
{
    osmlayer *lold = m_overlay;
    m_overlay = NULL;

    // Destroy the orig
    if (lold)
    {
        remove_event_listener(lold);
        delete lold;
    }

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

void mapctrl::goto_cursor()
{
    // GPSd not connected
    if (!gpsd_connected())
        return;

    // No valid fix
    if (!m_gpsdlayer->valid())
        return;

    // Convert GPS position to pixel position on the map for the current zoom
    // level
    point2d<unsigned long> ppx(utils::wsg842px(m_viewport.z(), m_gpsdlayer->pos()));

    // Center the viewport over the GPS position
    unsigned long x = (ppx.x() < (m_viewport.w()/2)) ? 0 : ppx.x()-(m_viewport.w()/2);
    unsigned long y = (ppx.y() < (m_viewport.h()/2)) ? 0 : ppx.y()-(m_viewport.h()/2);
    m_viewport.x(x);
    m_viewport.y(y);

    // Redraw
    refresh();
}

point2d<double> mapctrl::mousepos()
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
    return point2d<double> (
            utils::px2wsg84(m_viewport.z(), point2d<unsigned long>(m_viewport.x()+px, m_viewport.y()+py)));
}

void mapctrl::refresh()
{
    // Quote from the doc: The public method Fl_Widget::redraw() simply does
    // Fl_Widget::damage(FL_DAMAGE_ALL)
    redraw();
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

bool mapctrl::gpsd_evt_motion(const gpsdlayer::event_motion *e)
{
    // Track recording on, add current position
    if (m_recordtrack)
        m_gpxlayer->add_trackpoint(e->pos());
    
    // Center the viewport over the current position
    if (m_lockcursor)
        goto_cursor();
    else 
        refresh();

    // Notify any listeners about the change
    event_notify en;
    fire(&en);

    return true;
}

bool mapctrl::gpsd_evt_status(const gpsdlayer::event_status *e)
{
    refresh();
    event_notify en;
    fire(&en);
    return true;
}

bool mapctrl::osm_evt_notify(const osmlayer::event_notify *e)
{
    refresh();
    return true;
}

bool mapctrl::gpx_evt_notify(const gpxlayer::event_notify *e)
{
    refresh();
    return true;
}

bool mapctrl::marker_evt_notify(const markerlayer::event_notify *e)
{
    refresh();
    return true;
}

int mapctrl::handle_move(int event)
{
    // Save the current mouse position
    m_mousepos.x(Fl::event_x()-x());
    m_mousepos.y(Fl::event_y()-y());

    // Fire notification event
    event_notify en;
    fire(&en);

    return 1;
}

int mapctrl::handle_enter(int event)
{
    fl_cursor(FL_CURSOR_CROSS);
    return 1;
}

int mapctrl::handle_leave(int event)
{
    fl_cursor(FL_CURSOR_DEFAULT);
    return 1;
}

int mapctrl::handle_push(int event)
{
    // Focus this widget when pushed
    take_focus();

    // Drag-mode (right mouse button)? Change mouse cursor
    if (Fl::event_state(FL_BUTTON3) != 0)
        fl_cursor(FL_CURSOR_MOVE);

    // fire mouse event for the layers
    int button = layer::event_mouse::BUTTON_MIDDLE;
    if (Fl::event_button() == FL_LEFT_MOUSE)
        button = layer::event_mouse::BUTTON_LEFT;
    else if (Fl::event_button() == FL_RIGHT_MOUSE)
        button = layer::event_mouse::BUTTON_RIGHT;

    layer::event_mouse me(
            m_viewport, 
            layer::event_mouse::ACTION_PRESS,
            button, 
            vp_relative(point2d<int>(Fl::event_x(), Fl::event_y())));

    fire(&me);

    return 1;
}

int mapctrl::handle_release(int event)
{
    // End of drag mode, allow downloading of tiles
    if (m_basemap)
        m_basemap->dlenable(true);
    if (m_overlay)
        m_overlay->dlenable(true);

    // Cursor reset
    fl_cursor(FL_CURSOR_CROSS);

    // Mouse event for the layers
    int button = layer::event_mouse::BUTTON_MIDDLE;
    if (Fl::event_button() == FL_LEFT_MOUSE)
        button = layer::event_mouse::BUTTON_LEFT;
    else if (Fl::event_button() == FL_RIGHT_MOUSE)
    {
        button = layer::event_mouse::BUTTON_RIGHT;
    }

    layer::event_mouse me(
            m_viewport, 
            layer::event_mouse::ACTION_RELEASE,
            button, 
            vp_relative(point2d<int>(Fl::event_x(), Fl::event_y())));

    fire(&me);

    return 1;
}

int mapctrl::handle_drag(int event)
{
    // Move the viewport
    if (Fl::event_state(FL_BUTTON3) != 0)
    {
        // Calculate the delta with the last mouse position and save the
        // current mouse position
        int dx = m_mousepos.x() - (Fl::event_x()-x());
        int dy = m_mousepos.y() - (Fl::event_y()-y());

        m_mousepos.x(Fl::event_x()-x());
        m_mousepos.y(Fl::event_y()-y());

        // No tile downloading when dragging
        if (m_basemap)
            m_basemap->dlenable(false);
        if (m_overlay)
            m_overlay->dlenable(false);

        // Move the viewport accordingly and redraw
        m_viewport.move((long)dx, (long)dy); 
        refresh();
    }
    // Drag event for the layers
    else
    {
        int button = layer::event_mouse::BUTTON_MIDDLE;
        if (Fl::event_state(FL_BUTTON1) != 0)
            button = layer::event_mouse::BUTTON_LEFT;
        else if (Fl::event_state(FL_BUTTON3) != 0)
            button = layer::event_mouse::BUTTON_RIGHT;

        layer::event_mouse me(
                m_viewport,
                layer::event_mouse::ACTION_DRAG,
                button,
                vp_relative(point2d<int>(Fl::event_x(), Fl::event_y())));

        fire(&me);
    }

    return 1;
}

int mapctrl::handle_mousewheel(int event)
{
    // Outside this widget
    if (!Fl::event_inside(this))
        return 0;

    // No negative zoomlevel allowed
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
    
    // Refresh and notify
    refresh();
    event_notify e;
    fire(&e);

    return 1;
}

int mapctrl::handle_keyboard(int event)
{
    int ret = 0;

    if (std::string(Fl::event_text()) == "-")
    {
        if (m_viewport.z() != 0)
        {
            m_viewport.z(
                    m_viewport.z()-1,
                    m_viewport.w()/2,
                    m_viewport.h()/2);

            refresh();
            event_notify e;
            fire(&e);
            ret = 1;
        }
    }
    else if (std::string(Fl::event_text()) == "+")
    {
        m_viewport.z(
                m_viewport.z()+1,
                m_viewport.w()/2,
                m_viewport.h()/2);

        refresh();
        event_notify e;
        fire(&e);
        ret = 1;
    }
    else if (Fl::event_key(FL_Delete))
    {
        if (gpx_wpselected())
        {
            gpx_wpdelete();
            event_notify e;
            fire(&e);
            ret = 1;
        }
    }
    else if (Fl::event_key(FL_Left))
    {
        m_viewport.move(-PXMOTION, 0);
        refresh();
        ret = 1;
    }
    else if (Fl::event_key(FL_Right))
    {
        m_viewport.move(PXMOTION, 0);
        refresh();
        ret = 1;
    }
    else if (Fl::event_key(FL_Up))
    {
        m_viewport.move(0, -PXMOTION);
        refresh();
        ret = 1;
    }
    else if (Fl::event_key(FL_Down))
    {
        m_viewport.move(0, PXMOTION);
        refresh();
        ret = 1;
    }

    return ret;
}

int mapctrl::handle(int event) 
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
                handle_push(event); 

                // The push event always needs to return 1, otherwise dragging
                // will not work
                ret = 1;
                break;
            }
        case FL_RELEASE:
            {
                ret = handle_release(event);
                break;
            }
        case FL_DRAG: 
            {
                ret = handle_drag(event);
                break;
            }
        case FL_MOUSEWHEEL:
            {
                ret = handle_mousewheel(event);
                break;
            }
        case FL_FOCUS:
            {
                // Focus is accepted
                ret = 1;
                break;
            }
        case FL_KEYBOARD:
            {
                ret = handle_keyboard(event);
                break;
            }
    }

    return ret;
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
    //fl_rectf(0, 0, m_viewport.w(), m_viewport.h(), 80, 80, 80);
    fl_rectf(0, 0, m_viewport.w(), m_viewport.h(), 0, 0, 200);

    // Draw the basemap
    if (m_basemap)
        m_basemap->draw(m_viewport, m_offscreen);

    // Draw the overlay
    if (m_overlay)
        m_overlay->draw(m_viewport, m_offscreen);

    // Draw the gpx layer
    if (m_gpxlayer)
        m_gpxlayer->draw(m_viewport, m_offscreen);

    // Draw the marker layer
    if (m_markerlayer)
        m_markerlayer->draw(m_viewport, m_offscreen);

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

