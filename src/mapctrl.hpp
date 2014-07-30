#ifndef MAPCTRL_HPP
#define MAPCTRL_HPP

#include <vector>
#include <string>
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "viewport.hpp"
#include "point.hpp"
#include "osmlayer.hpp"
#include "gpxlayer.hpp"
#include "markerlayer.hpp"
#include "scalelayer.hpp"
#include "gpsdlayer.hpp"
#include "gfx.hpp"

class mapctrl : public Fl_Widget, public event_listener, public event_generator
{
    public:
        mapctrl(int x, int y, int w, int h, const char *label);
        ~mapctrl();

        // FLTK event handling routine
        int handle(int event);

        // Map and overlay configuration
        void basemap(
                const std::string& name, 
                const std::string& url, 
                unsigned int zmin, 
                unsigned int zmax, 
                unsigned int parallel,
                int imgtype);
        void overlay(
                const std::string& name, 
                const std::string& url, 
                unsigned int zmin, 
                unsigned int zmax, 
                unsigned int parallel,
                int imgtype);
        void clear_overlay();

        // GPSd configuration
        bool gpsd_connected();
        void gpsd_connect(const std::string& host, const std::string& port);
        void gpsd_disconnect();
        void gpsd_lock(bool start);
        void gpsd_record(bool start);
        int gpsd_mode();

        // GPX configuration
        void gpx_loadtrack(const std::string& path);
        void gpx_savetrack(const std::string& path);
        void gpx_cleartrack();
        bool gpx_wpselected();
        void gpx_wpdelete();
        void gpx_selection_get(std::vector<gpxlayer::waypoint>& waypoints);
        void gpx_selection_set(const std::vector<gpxlayer::waypoint>& waypoints); 
        double gpx_trip();
        void gpx_showwpmarkers(bool s);
        std::string gpx_trackname();
        
        // Viewport control
        unsigned int zoom();
        void zoom(unsigned int z);
        void goto_cursor();
        void goto_pos(const point2d<double> &pwsg84);
        point2d<double> mousepos();
       
        // Marker handling
        void marker_add(const point2d<double> &pmerc, size_t id);
        size_t marker_add(const point2d<double> &pmerc);
        void marker_remove(size_t id);

        // Event classes
        class event_notify;
    private:
        // Pixel delta for keyborad map motion commands
        static const int PXMOTION = 15;

        // Utility methods
        void refresh();

        // Widget event handling routines
        int handle_move(int event);
        int handle_enter(int event);
        int handle_leave(int event);
        int handle_push(int event);
        int handle_release(int event);
        int handle_drag(int event);
        int handle_mousewheel(int event);
        int handle_keyboard(int event);
        point2d<int> vp_relative(const point2d<int>& pos);
        bool vp_inside(const point2d<int>& pos);

        // GPSd-layer event handlers
        bool gpsd_evt_motion(const gpsdlayer::event_motion *e);
        bool gpsd_evt_status(const gpsdlayer::event_status *e);

        // Basemap-layer event handlers
        bool osm_evt_notify(const osmlayer::event_notify *e);

        // GPX-layer event handlers
        bool gpx_evt_notify(const gpxlayer::event_notify *e);

        // Marker layer event handlers
        bool marker_evt_notify(const markerlayer::event_notify *e);

        // Layers
        osmlayer *m_basemap;
        osmlayer *m_overlay;
        scalelayer *m_scale;
        gpxlayer *m_gpxlayer;
        markerlayer *m_markerlayer;
        gpsdlayer *m_gpsdlayer;

        point2d<int> m_mousepos;
        viewport m_viewport;
        viewport m_viewport_map;
        fgfx::canvas m_offscreen;
        fgfx::canvas m_offscreen_map;
        bool m_lockcursor;
        bool m_recordtrack;

    protected:
        void draw();
};

class mapctrl::event_notify : public event_base
{
    public:
        event_notify() {};
        ~event_notify() {};
};

#endif // MAPCTRL_HPP

