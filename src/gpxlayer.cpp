#include <cmath>
#include <sstream>
#include <FL/x.H>
#include <FL/fl_draw.H>
#include "utils.hpp"
#include "settings.hpp"
#include "point.hpp"
#include "gpxlayer.hpp"

gpxlayer::gpxlayer(const std::string &path) :
    layer()
{
    name(std::string("Unnamed GPX layer"));
    TiXmlDocument doc(path);
    if (!doc.LoadFile())
        return;

    parsetree(doc.RootElement());
};

gpxlayer::~gpxlayer()
{
    ;
};

void gpxlayer::draw(const viewport &viewport, canvas &os)
{
    unsigned int lastx = 0, lasty = 0, lastday = 0;
    settings &settings = settings::get_instance();

    // Get track colors and pixel width from the settings
    int trk_color0, trk_color1, trk_width;
    //settings.getopt(std::string("gpx::linecolor0"), trk_color0);
    //settings.getopt(std::string("gpx::linecolor1"), trk_color1);
    //settings.getopt(std::string("gpx::linewidth"), trk_width);

    trk_color0 = 0xff0000;
    trk_color1 = 0x00ff00;
    trk_width = 2;

    // Set track color and width
    Fl_Color old_color = fl_color();
    fl_color(((trk_color0>>16) & 0xff), ((trk_color0>>8) & 0xff), trk_color0 & 0xff);
    fl_line_style(FL_SOLID, trk_width, NULL);

    int colorswitch = 0;
    std::vector<gpx_trkpt>::iterator iter;
    for (iter=m_trkpts.begin();iter!=m_trkpts.end();++iter) {

        point<unsigned int> px;
        utils::merc2px(viewport.z(), point<double>((*iter).lon, (*iter).lat), px);

        if (iter == m_trkpts.begin()) {
            lastx = px.get_x();
            lasty = px.get_y();
            lastday = (*iter).time / (60*60*24);
        }

        // Track highlighting on a per day basis (should probably be made
        // configurable). Track color0 and track color1 are simply toggled.
        if (((*iter).time / (60*60*24)) != lastday) {
            if ((colorswitch % 2) == 0)
                fl_color(((trk_color0>>16) & 0xff), ((trk_color0>>8) & 0xff), trk_color0 & 0xff);
            else
                fl_color(((trk_color1>>16) & 0xff), ((trk_color1>>8) & 0xff), trk_color1 & 0xff);

            colorswitch++;
        }

        // If either the last point or the current point are inside the viewport
        // area then draw. However, this doesn' work for lines which just cross
        // the viewport area without any points inside. A better aproach is needed
        if (((lastx >= viewport.x()) && (lastx < (viewport.x()+viewport.w())) && 
             (lasty >= viewport.y()) && (lasty < (viewport.y()+viewport.h()))) ||
             ((px.get_x() >= viewport.x()) && (px.get_x() < (viewport.x()+viewport.w())) && 
              (px.get_y() >= viewport.y()) && (px.get_y() < (viewport.y()+viewport.h())))) {
            fl_line(
                    (int)(lastx-viewport.x()), 
                    (int)(lasty-viewport.y()), 
                    (int)(px.get_x()-viewport.x()), 
                    (int)(px.get_y()-viewport.y()));
        }

        lastx = px.get_x();
        lasty = px.get_y();
        lastday = (*iter).time / (60*60*24);
    }

    // Reset line style and color
    fl_line_style(0);
    fl_color(old_color);
}

int gpxlayer::parsetree(TiXmlNode *parent)
{
    int t = parent->Type();
    gpx_trkpt p;

    if (t != TiXmlNode::TINYXML_ELEMENT) {
        return 0;
    }

    std::string val(parent->Value());

    // Handle trackpoint
    if (val.compare("trkpt") == 0) {
        double lat, lon;
        parent->ToElement()->Attribute("lat", &lat);
        parent->ToElement()->Attribute("lon", &lon);

        // Convert latitude and longitude to mercator coordinates
        point<double> merc;
        utils::gps2merc(point<double>(lon, lat), merc);

        p.lon = merc.get_x();
        p.lat = merc.get_y();
        p.time = 0;
        p.ele = 0;

        // Look for "time" and "ele" childnodes
        TiXmlNode *child;
        for (child = parent->FirstChild(); child != NULL; child = child->NextSibling()) {
            if (std::string(child->Value()).compare("time") == 0) {
                p.time = iso8601_2timet(std::string(child->ToElement()->GetText()));
            }
            else if (std::string(child->Value()).compare("ele") == 0) {
                std::istringstream iss(child->ToElement()->GetText());
                iss >> p.ele;
            }
        }

        /* Add the point to the list */
        m_trkpts.push_back(p);
    } 
    // Handle trackname
    else if (val.compare("name") == 0) {
        name(std::string(parent->ToElement()->GetText()));
    }

    // Recurse the rest of the subtree
    TiXmlNode *child;
    for (child = parent->FirstChild(); child != NULL; child = child->NextSibling()) {
        parsetree(child);
    }

    return 0;
}

time_t gpxlayer::iso8601_2timet(const std::string iso)
{
    struct tm stm;
    strptime(iso.c_str(), "%FT%T%z", &stm);

    return mktime(&stm);
}

