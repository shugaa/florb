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

        void click(const viewport& vp, point2d<unsigned long> px);
        void key();
        void drag(const viewport& vp, point2d<unsigned long> px);
        void push(const viewport& vp, point2d<unsigned long> px);
        
        bool handle_evt_mouse(const layer_mouseevent* evt);
        bool handle_evt_key(const layer_keyevent* evt);

        void draw(const viewport& vp, canvas& os);

    private:
        struct gpx_trkpt {
            double lat;
            double lon;
            double ele;
            time_t time;
        };

        int parsetree(TiXmlNode *parent);
        time_t iso8601_2timet(const std::string iso);

        std::vector<gpx_trkpt> m_trkpts;
        size_t m_highlight;
        bool m_dragmode;
        point2d<unsigned long> m_pushpos;
        size_t m_pushidx;
        gpx_trkpt m_dragging;
};

#endif // GPXLAYER_HPP

