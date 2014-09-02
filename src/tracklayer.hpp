#ifndef TRACKLAYER_HPP
#define TRACKLAYER_HPP

#include <string>
#include <iostream>
#include <tinyxml2.h>
#include <vector>
#include <ctime>
#include <layer.hpp>
#include "viewport.hpp"
#include "point.hpp"

namespace florb {

    class tracklayer : public florb::layer
    {
        public:
            tracklayer(const std::string &path);
            tracklayer();
            ~tracklayer();

            class event_notify;
            class waypoint;

            bool handle_evt_mouse(const layer::event_mouse* evt);
            bool handle_evt_key(const layer::event_key* evt);

            bool draw(const viewport& vp, florb::canvas& os);
            void load_track(const std::string &path);
            void save_track(const std::string &path);
            void clear_track();
            void add_trackpoint(const florb::point2d<double>& p);

            size_t selected();
            void selection_get(std::vector<waypoint>& waypoints);
            void selection_set(const std::vector<waypoint>& waypoints);
            void selection_delete();

            double trip();
            void showwpmarkers(bool s);

        private:
            static const unsigned int wp_hotspot = 6;
            static const std::string trackname;

            struct gpx_trkpt {
                double lat;
                double lon;
                double ele;
                time_t time;
            };
            struct selection {
                // Multiselect
                bool multiselect;

                // Dragging
                bool dragging;
                florb::point2d<double> dragorigin;
                florb::point2d<double> dragcurrent;

                std::vector< std::vector<gpx_trkpt>::iterator > waypoints;
            };

            void notify();

            bool press(const layer::event_mouse* evt);
            bool release(const layer::event_mouse* evt);
            bool drag(const layer::event_mouse* evt);
            bool key(const layer::event_key* evt);
            void trip_update();
            void trip_calcall();

            bool clipline(florb::point2d<double> &p1, florb::point2d<double> &p2, florb::point2d<double> r1, florb::point2d<double> r2, bool &p1clip, bool &p2clip);

            void parsetree(tinyxml2::XMLNode *parent);

            std::vector<gpx_trkpt> m_trkpts;
            selection m_selection;
            long double m_trip;
            bool m_showwpmarkers;
    };

    class tracklayer::event_notify : public event_base
    {
        public:
            event_notify() {};
            ~event_notify() {};
    };

    class tracklayer::waypoint
    {
        public:
            waypoint(double lon, double lat, double ele, time_t ti) :
                m_lon(lon),
                m_lat(lat),
                m_ele(ele),
                m_time(ti) {};
            ~waypoint() {};

            double lon() const { return m_lon; };
            void lon(double l) { m_lon = l; };

            double lat() const { return m_lat; };
            void lat(double l) { m_lat = l; };

            double elevation() const { return m_ele; };
            void elevation(double ele) { m_ele = ele; };

            time_t time() const { return m_time; };
            void time(double t) { m_time = t; };

        private:
            double m_lon;
            double m_lat;
            double m_ele;
            time_t m_time;
    };
};

#endif // TRACKLAYER_HPP

