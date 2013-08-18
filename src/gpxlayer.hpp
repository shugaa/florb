#ifndef GPXLAYER_HPP
#define GPXLAYER_HPP

#include <string>
#include <iostream>
#include <tinyxml.h>
#include <vector>
#include <ctime>
#include <layer.hpp>
#include "viewport.hpp"

class gpxlayer : public layer
{
    public:
        gpxlayer(const std::string &path);
        ~gpxlayer();

        void draw(const viewport &viewport,canvas &os);

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
};

#endif // GPXLAYER_HPP

