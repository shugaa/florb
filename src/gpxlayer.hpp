#ifndef GPXLAYER_HPP
#define GPXLAYER_HPP

#include <string>
#include <iostream>
#include <tinyxml2.h>
#include <vector>
#include <ctime>
#include <layer.hpp>
#include "viewport.hpp"
#include "point.hpp"

class gpxlayer : public layer
{
    public:
        gpxlayer(const std::string &path);
        gpxlayer();
        ~gpxlayer();

        bool handle_evt_mouse(const layer::event_mouse* evt);
        bool handle_evt_key(const layer::event_key* evt);

        void draw(const viewport& vp, canvas& os);
        void load_track(const std::string &path);
        void save_track(const std::string &path);
        void clear_track();
        void add_trackpoint(const point2d<double>& p);


        bool selected();
        point2d<double> selection_pos();
        void selection_pos(const point2d<double>& p);

        double selection_elevation();
        void selection_elevation(double e);
        void selection_delete();
    
        double trip();
        void showwpmarkers(bool s);

        class event_notify;

    private:
        struct gpx_trkpt {
            double lat;
            double lon;
            double ele;
            time_t time;
        };
        struct selection {
            bool highlight;
            bool dragging;
            std::vector<gpx_trkpt>::iterator it;
        };

        void notify();

        bool press(const layer::event_mouse* evt);
        bool release(const layer::event_mouse* evt);
        bool drag(const layer::event_mouse* evt);
        bool key(const layer::event_key* evt);
        void trip_update();
        void trip_calcall();

        bool clipline(point2d<double> &p1, point2d<double> &p2, point2d<double> r1, point2d<double> r2, bool &p1clip, bool &p2clip);

        void parsetree(tinyxml2::XMLNode *parent);

        std::vector<gpx_trkpt> m_trkpts;
        selection m_selection;
        long double m_trip;
        bool m_showwpmarkers;
};

class gpxlayer::event_notify : public event_base
{
    public:
        event_notify() {};
        ~event_notify() {};
};

#endif // GPXLAYER_HPP

