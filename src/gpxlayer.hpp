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

        
        void key();
        
        
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
        struct selection {
            bool highlight;
            gpx_trkpt trkpt;
            std::vector<gpx_trkpt>::iterator it;
        };

        bool press(const layer_mouseevent* evt);
        bool release(const layer_mouseevent* evt);
        bool drag(const layer_mouseevent* evt);
        bool key(const layer_keyevent* evt);

        int parsetree(TiXmlNode *parent);
        time_t iso8601_2timet(const std::string iso);

        std::vector<gpx_trkpt> m_trkpts;
        size_t m_highlight;

        selection m_selection;
};

#endif // GPXLAYER_HPP

