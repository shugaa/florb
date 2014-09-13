#ifndef WGT_MAP_HPP
#define WGT_MAP_HPP

#include <vector>
#include <string>
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "viewport.hpp"
#include "point.hpp"
#include "osmlayer.hpp"
#include "tracklayer.hpp"
#include "markerlayer.hpp"
#include "scalelayer.hpp"
#include "gpsdlayer.hpp"
#include "areaselectlayer.hpp"
#include "gfx.hpp"

namespace florb
{
    class wgt_map : public Fl_Widget, public event_listener, public event_generator
    {
        public:
            wgt_map(int x, int y, int w, int h, const char *label);
            ~wgt_map();

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
            void gpx_selection_get(std::vector<florb::tracklayer::waypoint>& waypoints);
            void gpx_selection_set(const std::vector<florb::tracklayer::waypoint>& waypoints); 
            double gpx_trip();
            void gpx_showwpmarkers(bool s);
            std::string gpx_trackname();

            // Viewport control
            unsigned int zoom();
            void zoom(unsigned int z);
            void goto_cursor();
            void goto_pos(const florb::point2d<double> &pwsg84);
            florb::point2d<double> mousepos();

            // Marker handling
            void marker_add(const florb::point2d<double> &pmerc, size_t id);
            size_t marker_add(const florb::point2d<double> &pmerc);
            void marker_remove(size_t id);

            // Area selection
            void select_area(const std::string& caption);
            void select_clear();

            // Event classes
            class event_notify;
            class event_endselect;
        private:
            // Pixel delta for keyborad map motion commands
            static const int PXMOTION = 15;

            // Utility methods
            void refresh();
            bool dragging();
            void dragging(bool d);
            bool dirty();
            void dirty(bool d);

            // Widget event handling routines
            int handle_move(int event);
            int handle_enter(int event);
            int handle_leave(int event);
            int handle_push(int event);
            int handle_release(int event);
            int handle_drag(int event);
            int handle_mousewheel(int event);
            int handle_keyboard(int event);
            florb::point2d<int> vp_relative(const florb::point2d<int>& pos);
            bool vp_inside(const florb::point2d<int>& pos);

            // GPSd-layer event handlers
            bool gpsd_evt_motion(const florb::gpsdlayer::event_motion *e);
            bool gpsd_evt_status(const florb::gpsdlayer::event_status *e);

            // Basemap-layer event handlers
            bool osm_evt_notify(const florb::osmlayer::event_notify *e);

            // GPX-layer event handlers
            bool gpx_evt_notify(const florb::tracklayer::event_notify *e);

            // Marker layer event handlers
            bool marker_evt_notify(const florb::markerlayer::event_notify *e);

            // Area selection layer event handlers
            bool areaselect_evt_done(const florb::areaselectlayer::event_done *e);
            bool areaselect_evt_notify(const florb::areaselectlayer::event_notify *e);

            // Layers
            florb::osmlayer *m_basemap;
            florb::osmlayer *m_overlay;
            florb::scalelayer *m_scale;
            florb::tracklayer *m_tracklayer;
            florb::markerlayer *m_markerlayer;
            florb::gpsdlayer *m_gpsdlayer;
            florb::areaselectlayer *m_areaselectlayer;

            florb::point2d<int> m_mousepos;
            viewport m_viewport;
            viewport m_viewport_off;
            florb::canvas m_offscreen;

            bool m_lockcursor;
            bool m_recordtrack;
            bool m_dragging;
            bool m_dirty;

        protected:
            void draw();
    };

    class wgt_map::event_notify : public event_base
    {
        public:
            event_notify() {};
            ~event_notify() {};
    };

    class wgt_map::event_endselect : public event_base
    {
        public:
            event_endselect(const viewport& vp) : m_vp(vp) {};
            ~event_endselect() {};

            const viewport& vp() const { return m_vp; };
        private:
            viewport m_vp;
    };

};

#endif // WGT_MAP_HPP

