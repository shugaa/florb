#include <cmath>
#include <iostream>
#include <FL/fl_draw.H>
#include <FL/x.H>
#include "settings.hpp"
#include "wgt_map.hpp"
#include "utils.hpp"

florb::wgt_map::wgt_map(int x, int y, int w, int h, const char *label) : 
    Fl_Widget(x, y, w, h, label),
    m_basemap(NULL),
    m_tracklayer(NULL),
    m_gpsdlayer(NULL),
    m_mousepos(0, 0),
    m_viewport(w, h),
    m_viewport_off(0, 0),
    m_offscreen(w, h),
    m_lockcursor(false),
    m_recordtrack(false),
    m_dragging(false),
    m_dirty(false)
{
    // Register event handlers for layer events
    register_event_handler<florb::wgt_map, gpsdlayer::event_status>(this, &florb::wgt_map::gpsd_evt_status);
    register_event_handler<florb::wgt_map, gpsdlayer::event_motion>(this, &florb::wgt_map::gpsd_evt_motion);
    register_event_handler<florb::wgt_map, osmlayer::event_notify>(this, &florb::wgt_map::osm_evt_notify);
    register_event_handler<florb::wgt_map, florb::tracklayer::event_notify>(this, &florb::wgt_map::gpx_evt_notify);
    register_event_handler<florb::wgt_map, markerlayer::event_notify>(this, &florb::wgt_map::marker_evt_notify);
    register_event_handler<florb::wgt_map, areaselectlayer::event_done>(this, &florb::wgt_map::areaselect_evt_done);
    register_event_handler<florb::wgt_map, areaselectlayer::event_notify>(this, &florb::wgt_map::areaselect_evt_notify);

    // Add a GPX layer
    try {
        m_tracklayer = new florb::tracklayer();
    } catch (...) {
        m_tracklayer = NULL;
        throw std::runtime_error(_("GPX error"));
    }

    m_tracklayer->add_event_listener(this);
    add_event_listener(m_tracklayer);

    // Add a marker layer
    try {
        m_markerlayer = new markerlayer();
    } catch (...) {
        m_markerlayer = NULL;
        throw std::runtime_error(_("Marker error"));
    }

    m_markerlayer->add_event_listener(this);
    add_event_listener(m_markerlayer);

    // Add a scale layer
    try {
        m_scale = new scalelayer();
    } catch (...) {
        m_scale = NULL;
        throw std::runtime_error(_("Scale error"));
    }

    // Add an area selection layer
    try {
        m_areaselectlayer = new areaselectlayer();
    } catch (...) {
        m_areaselectlayer = NULL;
        throw std::runtime_error(_("Selection error"));
    }

    m_areaselectlayer->enable(false);
    m_areaselectlayer->add_event_listener(this);
    add_event_listener(m_areaselectlayer);

    // Add a gpsdlayer
    m_gpsdlayer = new gpsdlayer();
    m_gpsdlayer->add_event_listener(this);

    // Connect to gpsd if configured
    florb::cfg_gpsd cfggpsd = florb::settings::get_instance()["gpsd"].as<florb::cfg_gpsd>();
    if (cfggpsd.enabled())
        gpsd_connect(cfggpsd.host(), cfggpsd.port());

    // Restore previous viewport
    florb::cfg_viewport cfgvp = florb::settings::get_instance()["viewport"].as<florb::cfg_viewport>();
    if (cfgvp.z() > m_viewport.z())
        m_viewport.z(cfgvp.z(), m_viewport.w()/2, m_viewport.h()/2);

    // set previous position
    goto_pos(florb::point2d<double>(cfgvp.lon(), cfgvp.lat()));
}

florb::wgt_map::~wgt_map()
{
    // Save viewport configuration
    florb::cfg_viewport cfgvp = florb::settings::get_instance()["viewport"].as<florb::cfg_viewport>();
    cfgvp.z(m_viewport.z());
    florb::point2d<double> pos = florb::utils::px2wsg84(
            m_viewport.z(), 
            florb::point2d<unsigned long>(m_viewport.x()+(m_viewport.w()/2), m_viewport.y()+(m_viewport.h()/2)));

    cfgvp.lon(pos.x());
    cfgvp.lat(pos.y());
    florb::settings::get_instance()["viewport"] = cfgvp;

    // Delete all layers if active
    if (m_basemap)
        delete m_basemap;

    if (m_overlay)
        delete m_overlay;

    if (m_tracklayer)
        delete m_tracklayer;

    if (m_scale)
        delete m_scale;

    if (m_markerlayer)
        delete m_markerlayer;

    if (m_gpsdlayer)
        delete m_gpsdlayer;

    if (m_areaselectlayer)
        delete m_areaselectlayer;
}

void florb::wgt_map::goto_pos(const florb::point2d<double> &pwsg84)
{
    // set previous position
    florb::point2d<unsigned long> ppx(florb::utils::wsg842px(zoom(), pwsg84));

    unsigned long dx = m_viewport.w()/2;
    unsigned long dy = m_viewport.h()/2;

    m_viewport.x((ppx.x() >= dx) ? (ppx.x()-dx) : 0);
    m_viewport.y((ppx.y() >= dy) ? (ppx.y()-dy) : 0);

    refresh();
}

void florb::wgt_map::marker_add(const florb::point2d<double> &pmerc, size_t id)
{
    if (!m_markerlayer)
        throw std::runtime_error(_("Marker error"));

    m_markerlayer->add(pmerc, id);
}

size_t florb::wgt_map::marker_add(const florb::point2d<double> &pmerc)
{
    if (!m_markerlayer)
        throw std::runtime_error(_("Marker error"));

    return m_markerlayer->add(pmerc);
}

void florb::wgt_map::marker_remove(size_t id)
{
    if (!m_markerlayer)
        throw std::runtime_error(_("Marker error"));

    m_markerlayer->remove(id);
}

void florb::wgt_map::select_area(const std::string& caption)
{
    // Disabe florb::tracklayer
    m_tracklayer->enable(false);

    // Enable areaselectlayer
    m_areaselectlayer->enable(true);
}

void florb::wgt_map::select_clear()
{
    m_areaselectlayer->clear();
}

void florb::wgt_map::gpx_loadtrack(const std::string& path)
{
    if (!m_tracklayer)
        throw 0;

    m_tracklayer->load_track(path);
}

void florb::wgt_map::gpx_savetrack(const std::string& path)
{
    if (!m_tracklayer)
        throw 0;

    m_tracklayer->save_track(path);
}

void florb::wgt_map::gpx_cleartrack()
{
    if (!m_tracklayer)
        throw 0;

    m_tracklayer->clear_track();
}

bool florb::wgt_map::gpx_wpselected()
{
    if (!m_tracklayer)
        throw 0;

    return (m_tracklayer->selected() > 0);
}

void florb::wgt_map::gpx_selection_get(std::vector<florb::tracklayer::waypoint>& waypoints)
{
    if (!m_tracklayer)
        throw 0;

    m_tracklayer->selection_get(waypoints);
}
        
void florb::wgt_map::gpx_selection_set(const std::vector<florb::tracklayer::waypoint>& waypoints)
{
    if (!m_tracklayer)
        throw 0;

    m_tracklayer->selection_set(waypoints);
    refresh();
}

void florb::wgt_map::gpx_wpdelete()
{
    if (!m_tracklayer)
        throw 0;

    m_tracklayer->selection_delete();    
}

double florb::wgt_map::gpx_trip()
{
    if (!m_tracklayer)
        throw 0;

    return m_tracklayer->trip();
}

void florb::wgt_map::gpx_showwpmarkers(bool s)
{
    if (!m_tracklayer)
        throw 0;

    m_tracklayer->showwpmarkers(s);
}

std::string florb::wgt_map::gpx_trackname()
{
    if (!m_tracklayer)
        throw 0;

    return m_tracklayer->name();
}

bool florb::wgt_map::gpsd_connected()
{
    return m_gpsdlayer->connected();
}

void florb::wgt_map::gpsd_connect(const std::string& host, const std::string& port)
{
    gpsd_disconnect();

    try {
        m_gpsdlayer->connect(host, port);
    } catch (std::runtime_error& e) {
        throw e;
    }
}

void florb::wgt_map::gpsd_disconnect()
{
    // Disconnect
    m_gpsdlayer->disconnect();
    
    // Refresh display and notify
    refresh();
    event_notify e;
    fire(&e);
}

void florb::wgt_map::gpsd_record(bool start)
{
    m_recordtrack = start;
}

void florb::wgt_map::gpsd_lock(bool start)
{
    m_lockcursor = start;
    if (m_lockcursor)
        goto_cursor();
}

int florb::wgt_map::gpsd_mode()
{
    return m_gpsdlayer->mode();
}

void florb::wgt_map::basemap(
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

    // Invalidate map buffer
    m_viewport_off.w(0);

    // Redraw
    refresh();
}

void florb::wgt_map::overlay(
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

void florb::wgt_map::clear_overlay()
{
    osmlayer *lold = m_overlay;
    m_overlay = NULL;

    // Destroy the orig
    if (lold)
    {
        remove_event_listener(lold);
        delete lold;
    }

    // Invalidate map buffer
    m_viewport_off.w(0);

    refresh();
}

unsigned int florb::wgt_map::zoom()
{
    // Return current zoomlevel
    return m_viewport.z();
}

void florb::wgt_map::zoom(unsigned int z)
{
    // Set tne new zoomlevel
    m_viewport.z(z, m_viewport.w()/2, m_viewport.h()/2);
   
    // Request a redraw
    refresh();
}

void florb::wgt_map::goto_cursor()
{
    // GPSd not connected
    if (!gpsd_connected())
        return;

    // No valid fix
    if (!m_gpsdlayer->valid())
        return;

    goto_pos(m_gpsdlayer->pos());
}

florb::point2d<double> florb::wgt_map::mousepos()
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
    return florb::point2d<double> (
            florb::utils::px2wsg84(m_viewport.z(), florb::point2d<unsigned long>(m_viewport.x()+px, m_viewport.y()+py)));
}

void florb::wgt_map::refresh()
{
    // Quote from the doc: The public method Fl_Widget::redraw() simply does
    // Fl_Widget::damage(FL_DAMAGE_ALL)
    redraw();
}

void florb::wgt_map::dragging(bool d)
{
    m_dragging = d;
}

bool florb::wgt_map::dragging()
{
    return m_dragging;
}

void florb::wgt_map::dirty(bool d)
{
    m_dirty = d;
}

bool florb::wgt_map::dirty()
{
    return m_dirty;
}

florb::point2d<int> florb::wgt_map::vp_relative(const florb::point2d<int>& pos)
{
    // Convert to widget-relative coordinates first
    florb::point2d<int> ret(pos.x()-x(), pos.y()-y());

    // Calculate the widget <-> viewport delta and get the viewport-absolute
    // return coordinates
    if (w() > (int)m_viewport.w())
        ret[0] -= (w() - (int)m_viewport.w())/2;
    if (h() > (int)m_viewport.h())
        ret[1] -= (h() - (int)m_viewport.h())/2;

    return ret;
}

bool florb::wgt_map::vp_inside(const florb::point2d<int>& pos)
{
    florb::point2d<int> vprel(vp_relative(pos));

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

bool florb::wgt_map::gpsd_evt_motion(const gpsdlayer::event_motion *e)
{
    dirty(true);

    // Track recording on, add current position
    if (m_recordtrack)
        m_tracklayer->add_trackpoint(e->pos());
    
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

bool florb::wgt_map::gpsd_evt_status(const gpsdlayer::event_status *e)
{
    dirty(true);
    refresh();
    event_notify en;
    fire(&en);
    return true;
}

bool florb::wgt_map::osm_evt_notify(const osmlayer::event_notify *e)
{
    dirty(true);
    refresh();
    return true;
}

bool florb::wgt_map::gpx_evt_notify(const florb::tracklayer::event_notify *e)
{
    dirty(true);
    refresh();

    // Make sure the trip display is updated
    event_notify en;
    fire(&en);

    return true;
}

bool florb::wgt_map::marker_evt_notify(const markerlayer::event_notify *e)
{
    dirty(true);
    refresh();
    return true;
}

bool florb::wgt_map::areaselect_evt_done(const areaselectlayer::event_done *e)
{
    // disable areaselectlayer
    m_areaselectlayer->enable(false);

    // Enable tracklayer
    m_tracklayer->enable(true);

    // Fire event
    event_endselect en(e->vp());
    fire(&en);    

    return true;
}

bool florb::wgt_map::areaselect_evt_notify(const areaselectlayer::event_notify *e)
{
    dirty(true);
    refresh();
    return true;
}

int florb::wgt_map::handle_move(int event)
{
    // Save the current mouse position
    m_mousepos.x(Fl::event_x()-x());
    m_mousepos.y(Fl::event_y()-y());

    // Fire notification event
    event_notify en;
    fire(&en);

    return 1;
}

int florb::wgt_map::handle_enter(int event)
{
    fl_cursor(FL_CURSOR_CROSS);
    return 1;
}

int florb::wgt_map::handle_leave(int event)
{
    fl_cursor(FL_CURSOR_DEFAULT);
    return 1;
}

int florb::wgt_map::handle_push(int event)
{
    // Focus this widget when pushed
    take_focus();

    // Drag-mode (right mouse button)? Change mouse cursor
    if (Fl::event_state(FL_BUTTON3) != 0)
        fl_cursor(FL_CURSOR_MOVE);

    // fire mouse event for the layers
    int button = florb::layer::event_mouse::BUTTON_MIDDLE;
    if (Fl::event_button() == FL_LEFT_MOUSE)
        button = florb::layer::event_mouse::BUTTON_LEFT;
    else if (Fl::event_button() == FL_RIGHT_MOUSE)
        button = florb::layer::event_mouse::BUTTON_RIGHT;

    florb::layer::event_mouse me(
            m_viewport, 
            florb::layer::event_mouse::ACTION_PRESS,
            button, 
            vp_relative(florb::point2d<int>(Fl::event_x(), Fl::event_y())));

    fire(&me);

    return 1;
}

int florb::wgt_map::handle_release(int event)
{
    // End of drag mode, allow downloading of tiles
    if (m_basemap)
        m_basemap->dlenable(true);
    if (m_overlay)
        m_overlay->dlenable(true);

    dragging(false);

    // Cursor reset
    fl_cursor(FL_CURSOR_CROSS);

    // Mouse event for the layers
    int button = florb::layer::event_mouse::BUTTON_MIDDLE;
    if (Fl::event_button() == FL_LEFT_MOUSE)
        button = florb::layer::event_mouse::BUTTON_LEFT;
    else if (Fl::event_button() == FL_RIGHT_MOUSE)
    {
        button = florb::layer::event_mouse::BUTTON_RIGHT;
    }

    florb::layer::event_mouse me(
            m_viewport, 
            florb::layer::event_mouse::ACTION_RELEASE,
            button, 
            vp_relative(florb::point2d<int>(Fl::event_x(), Fl::event_y())));

    fire(&me);

    return 1;
}

int florb::wgt_map::handle_drag(int event)
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

        dragging(true);

        // Move the viewport accordingly and redraw
        m_viewport.move((long)dx, (long)dy); 
        refresh();
    }
    // Drag event for the layers
    else
    {
        int button = florb::layer::event_mouse::BUTTON_MIDDLE;
        if (Fl::event_state(FL_BUTTON1) != 0)
            button = florb::layer::event_mouse::BUTTON_LEFT;
        else if (Fl::event_state(FL_BUTTON3) != 0)
            button = florb::layer::event_mouse::BUTTON_RIGHT;

        florb::layer::event_mouse me(
                m_viewport,
                florb::layer::event_mouse::ACTION_DRAG,
                button,
                vp_relative(florb::point2d<int>(Fl::event_x(), Fl::event_y())));

        fire(&me);
    }

    return 1;
}

int florb::wgt_map::handle_mousewheel(int event)
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

int florb::wgt_map::handle_keyboard(int event)
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

int florb::wgt_map::handle(int event) 
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

void florb::wgt_map::draw() 
{
    // Make sure redraw() has been called previously
    if ((damage() & FL_DAMAGE_ALL) == 0) 
        return;

    // Resize the viewport to the current widget size before drawing
    m_viewport.w((unsigned long)w());
    m_viewport.h((unsigned long)h());

    // Check whether the master viewport is well within the offscreen viewport
    viewport vp_tmp(m_viewport_off);
    vp_tmp.intersect(m_viewport);

    // No, it isn't, update the offscreen viewport
    if (vp_tmp < m_viewport)
        dirty(true);

    // Map is dirty, force redraw
    if (dirty() && !dragging())
    {
        m_viewport_off = m_viewport; 
          
#if 0
        // We can't have a larger offscreen viewport atm because then the scale
        // would always be drawn outside the visible area.
        m_viewport_off.w(m_viewport.w()+256);
        m_viewport_off.h(m_viewport.h()+256);
#endif
        m_offscreen.resize(m_viewport_off.w(), m_viewport_off.h());

        m_offscreen.fgcolor(florb::color(0xc06e6e));
        m_offscreen.fillrect(0,0, m_offscreen.w(), m_offscreen.h());

        dirty(false);

        // Draw the basemap
        if (m_basemap)
        {
            if (!m_basemap->draw(m_viewport_off, m_offscreen))
                dirty(true);
        }

        // Draw the overlay
        if (m_overlay)
        {
            if (!m_overlay->draw(m_viewport_off, m_offscreen))
                dirty(true);
        }

        // Draw the scale
        if (!m_scale->draw(m_viewport_off, m_offscreen))
            dirty(true);

        // Draw the gpx layer
        if (!m_tracklayer->draw(m_viewport_off, m_offscreen))
            dirty(true);

        // Draw the marker layer
        if (!m_markerlayer->draw(m_viewport_off, m_offscreen))
            dirty(true);

        // Draw the gpsd layer
        if (!m_gpsdlayer->draw(m_viewport_off, m_offscreen))
            dirty(true);

        // Draw the areaselect layer
        if (!m_areaselectlayer->draw(m_viewport_off, m_offscreen))
            dirty(true);
    }

    // Calculate delta viewport / viewport_off
    int dpx_src = 0, dpy_src = 0, dpx_dst = 0, dpy_dst = 0;
    
    if (m_viewport_off.x() > m_viewport.x())
        dpx_dst = m_viewport_off.x() - m_viewport.x();
    else
        dpx_src = m_viewport.x() - m_viewport_off.x();

    if (m_viewport_off.y() > m_viewport.y())
        dpy_dst = m_viewport_off.y() - m_viewport.y();
    else
        dpy_src = m_viewport.y() - m_viewport_off.y();

    // Additional delta if viewport smaller than map widget 
    if (w() > (int)m_viewport.w())
        dpx_dst += (w() - (int)m_viewport.w())/2;
    if (h() > (int)m_viewport.h())
        dpy_dst += (h() - (int)m_viewport.h())/2;

    // Fill the canvas in case it is not entirely covered by the viewport image
    // to be generated.
    fl_rectf(x(), y(), w(), h(), 80, 80, 80);

    // Draw offscreen onto widget
    fl_copy_offscreen(
            x()+dpx_dst, 
            y()+dpy_dst, 
            m_viewport.w()-dpx_dst, 
            m_viewport.h()-dpy_dst, 
            m_offscreen.buf(), 
            dpx_src, 
            dpy_src);
}

