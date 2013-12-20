#ifndef GPXLAYER_HPP
#define GPXLAYER_HPP

#include <string>
#include <iostream>
#include <tinyxml.h>
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

        bool handle_evt_mouse(const layer_mouseevent* evt);
        bool handle_evt_key(const layer_keyevent* evt);

        void draw(const viewport& vp, canvas& os);
        void load_track(const std::string &path);
        void clear_track();
    
        double trip();
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

        bool press(const layer_mouseevent* evt);
        bool release(const layer_mouseevent* evt);
        bool drag(const layer_mouseevent* evt);
        bool key(const layer_keyevent* evt);
        void trip_update();
        void trip_calcall();

        bool clipline(point2d<double> &p1, point2d<double> &p2, point2d<double> r1, point2d<double> r2, bool &p1clip, bool &p2clip);

        int parsetree(TiXmlNode *parent);

        std::vector<gpx_trkpt> m_trkpts;
        selection m_selection;
        long double m_trip;
};

#endif // GPXLAYER_HPP

