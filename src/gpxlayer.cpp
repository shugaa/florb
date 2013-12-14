#include <cmath>
#include <sstream>
#include <FL/x.H>
#include <FL/fl_draw.H>
#include "utils.hpp"
#include "settings.hpp"
#include "point.hpp"
#include "gpxlayer.hpp"

gpxlayer::gpxlayer() :
    layer(),
    m_highlight(0),
    m_dragmode(false),
    m_pushidx(0)
{
    name(std::string("Unnamed GPX layer"));
    register_event_handler<gpxlayer, layer_mouseevent>(this, &gpxlayer::handle_evt_mouse);
    register_event_handler<gpxlayer, layer_keyevent>(this, &gpxlayer::handle_evt_key);
}

void gpxlayer::key()
{
    if (m_highlight < m_trkpts.size())
    {
        m_trkpts.erase(m_trkpts.begin()+m_highlight);
        m_highlight = m_trkpts.size();
        notify_observers();
    }
}

void gpxlayer::push(const viewport& vp, point2d<unsigned long> px)
{
    // Viewport-relative to absolute coordinate
    px.x(px.x()+vp.x());
    px.y(px.y()+vp.y());

    m_pushpos = px;
   
    // find an item index for pushpos
    size_t idx = 0;
    point2d<unsigned long> pushposc;
    std::vector<gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin();it!=m_trkpts.end();++it, idx++)
    {
        pushposc = utils::merc2px(vp.z(), point2d<double>((*it).lon, (*it).lat)); 

        // Check whether the click might refer to this point
        if (m_pushpos.x() >= (pushposc.x()+6))
            continue;
        if (m_pushpos.x() < ((pushposc.x()>=6) ? pushposc.x()-6 : 0))
            continue;
        if (m_pushpos.y() >= (pushposc.y()+6))
            continue;
        if (m_pushpos.y() < ((pushposc.y()>=6) ? pushposc.y()-6 : 0))
            continue;

        break;
    }

    m_pushidx = idx;
    if (idx < m_trkpts.size())
        m_dragging = m_trkpts[idx];
}

void gpxlayer::drag(const viewport& vp, point2d<unsigned long> px)
{
    // Viewport-relative to absolute coordinate
    px.x(px.x()+vp.x());
    px.y(px.y()+vp.y());

    // No item found
    if (m_pushidx >= m_trkpts.size()) 
    {
        return;
    }

    m_dragmode = true;

    int dx = 0, dy = 0;
    if (px.x() >= m_pushpos.x())
        dx = (px.x() - m_pushpos.x());
    else
        dx = -(m_pushpos.x() - px.x());
   
    if (px.y() >= m_pushpos.y())
        dy = (px.y() - m_pushpos.y());
    else
        dy = -(m_pushpos.y() - px.y());

    point2d<unsigned long> pixels = utils::merc2px(vp.z(), point2d<double>(m_dragging.lon, m_dragging.lat)); 
    point2d<double> merc = utils::px2merc(vp.z(), point2d<unsigned long>(pixels.x()+dx, pixels.y()+dy));
    m_trkpts[m_pushidx].lon = merc.x();
    m_trkpts[m_pushidx].lat = merc.y();

    notify_observers();
}

void gpxlayer::click(const viewport& vp, point2d<unsigned long> px)
{
    if (m_dragmode)
    {
        m_dragmode = false;
        return;
    }

    // Viewport-relative to absolute coordinate
    px.x(px.x()+vp.x());
    px.y(px.y()+vp.y());

    // Go through all existing points and check whether this click referred to
    // one of them
    size_t idx = 0;
    std::vector<gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin();it!=m_trkpts.end();++it, idx++)
    {
        point2d<unsigned long> pxc = utils::merc2px(vp.z(), point2d<double>((*it).lon, (*it).lat)); 

        // Check whether the click might refer to this point
        if (px.x() >= (pxc.x()+6))
            continue;
        if (px.x() < ((pxc.x()>=6) ? pxc.x()-6 : 0))
            continue;
        if (px.y() >= (pxc.y()+6))
            continue;
        if (px.y() < ((pxc.y()>=6) ? pxc.y()-6 : 0))
            continue;

        // Item is already highlighted, unhighlight
        if (idx == m_highlight) {
            m_highlight = m_trkpts.size();
            break;
        }

        // Item is not yet highlighted, highlight
        m_highlight = idx;
        break;
    }

    // This click referred to an existing point which is to be highlighted or
    // unhighlighted respectively
    if (idx < m_trkpts.size()) 
    {
        notify_observers();
        return;
    }

    // A new point is to be added
    // Try to convert the pixel position to mercator coordinate
    point2d<double> merc;
    try {
        merc = utils::px2merc(vp.z(), px);
    } catch (...) {
        return;
    }

    // Add the position to the list
    gpx_trkpt p;
    p.lon = merc.x();
    p.lat = merc.y();
    p.time = 0;
    p.ele = 0; 

    m_trkpts.push_back(p);

    // Highlight the last added point
    m_highlight = m_trkpts.size() - 1;

    // Indicate that this layer has changed
    notify_observers();
}

gpxlayer::gpxlayer(const std::string &path) :
    layer(),
    m_highlight(0),
    m_dragmode(false),
    m_pushidx(0)
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

bool gpxlayer::handle_evt_mouse(const layer_mouseevent* evt)
{
    std::cout << "SUPERDUPER mouse handler" << std::endl; 
    return true;
}

bool gpxlayer::handle_evt_key(const layer_keyevent* evt)
{
    std::cout << "SUPERDUPER key handler" << std::endl;
    return true;
}

void gpxlayer::draw(const viewport &vp, canvas &os)
{
    color color_track(0xff0000);
    color color_point(0x0000ff);
    color color_point_hl(0x00ff00);

    point2d<unsigned long> px_last;
    size_t idx = 0;

    std::vector<gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin();it!=m_trkpts.end();++it, idx++) 
    {
        // Convert mercator to pixel coordinates
        point2d<unsigned long> px = utils::merc2px(vp.z(), point2d<double>((*it).lon, (*it).lat)); 
        px.x(px.x() - vp.x());
        px.y(px.y() - vp.y());

        // Draw crosshair for this point
        if (m_highlight == idx)
            os.fgcolor(color_point_hl);
        else
            os.fgcolor(color_point);

        os.line(px.x()-6, px.y(), px.x()+6, px.y(), 1);
        os.line(px.x(), px.y()-6, px.x(), px.y()+6, 1);

        // No connecting line possible if this is the first point
        if (it == m_trkpts.begin()) {
            px_last = px;
            continue;
        }

        // Draw a connection between points
        os.fgcolor(color_track);
        os.line(px_last.x(), px_last.y(), px.x(), px.y(), 2);
        px_last = px;
    }
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
        point2d<double> merc;
        utils::gps2merc(point2d<double>(lon, lat), merc);

        p.lon = merc.x();
        p.lat = merc.y();
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

