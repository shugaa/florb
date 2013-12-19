#include <cmath>
#include <sstream>
#include <FL/x.H>
#include <FL/fl_draw.H>
#include "utils.hpp"
#include "settings.hpp"
#include "point.hpp"
#include "gpxlayer.hpp"

gpxlayer::gpxlayer() :
    layer()
{
    name(std::string("Unnamed GPX layer"));
    register_event_handler<gpxlayer, layer_mouseevent>(this, &gpxlayer::handle_evt_mouse);
    register_event_handler<gpxlayer, layer_keyevent>(this, &gpxlayer::handle_evt_key);
}

bool gpxlayer::key(const layer_keyevent* evt)
{
    if (evt->key() != layer_keyevent::KEY_DEL)
        return false;

    if ((m_selection.it != m_trkpts.end()) && m_selection.highlight)
    {
        m_trkpts.erase(m_selection.it);
        m_selection.it = m_trkpts.end();
        notify_observers();

        return true;
    }

    return false;
}

double gpxlayer::trip()
{
    double sum = 0.0;
    double sum_merc = 0.0;

    if (m_trkpts.size() <= 1)
        return sum;

    std::vector<gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin()+1;it!=m_trkpts.end();++it)
    {
        point2d<double> p1((*it).lon, (*it).lat);
        point2d<double> p2((*(it-1)).lon, (*(it-1)).lat);
        
        point2d<double> g1(utils::merc2gps(p1));
        point2d<double> g2(utils::merc2gps(p2));
         
        sum += utils::dist(g1, g2);
        sum_merc += utils::dist_merc(p1, p2);
    }

    std::cout << "sum gps  " << sum << std::endl;
    std::cout << "sum merc " << sum_merc << std::endl;


    return sum;
}

bool gpxlayer::press(const layer_mouseevent* evt)
{
    // Mouse push outside viewport area
    if ((evt->pos().x() < 0) || (evt->pos().y() < 0))
        return false;
    if ((evt->pos().x() >= (int)evt->vp().w()) || (evt->pos().y() >= (int)evt->vp().h()))
        return false;

    // Convert absolute map coordinate
    point2d<unsigned long> pxabs;
    pxabs.x(evt->pos().x() + evt->vp().x());
    pxabs.y(evt->pos().y() + evt->vp().y());

    // Find an existing item for this mouse position
    std::vector<gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin();it!=m_trkpts.end();++it)
    {
        point2d<unsigned long> cmp = utils::merc2px(evt->vp().z(), point2d<double>((*it).lon, (*it).lat)); 

        // Check whether the click might refer to this point
        if (pxabs.x() >= (cmp.x()+6))
            continue;
        if (pxabs.x() < ((cmp.x()>=6) ? cmp.x()-6 : 0))
            continue;
        if (pxabs.y() >= (cmp.y()+6))
            continue;
        if (pxabs.y() < ((cmp.y()>=6) ? cmp.y()-6 : 0))
            continue;

        break;
    }

    // New selection, turn highlighting off, it will be toggled on mouse button
    // release
    if ((m_selection.it != it))
        m_selection.highlight = false;
    
    // Either we have a valid iterator position or end() which is later on used
    // to tell whether we are actually dragging something around or not
    m_selection.it = it;

    return true;
}

bool gpxlayer::drag(const layer_mouseevent* evt)
{
    // Nothing to drag around
    if (m_selection.it == m_trkpts.end())
        return false;

    // Viewport-relative to absolute coordinate
    point2d<unsigned long> px(evt->pos().x(), evt->pos().y());

    if (evt->pos().x() < 0)
        px[0] = 0;
    else if (evt->pos().x() >= (int)evt->vp().w())
        px[0] = evt->vp().w()-1;
    if (evt->pos().y() < 0)
        px[1] = 0;
    else if (evt->pos().y() >= (int)evt->vp().h())
        px[1] = evt->vp().h()-1;

    px[0] += evt->vp().x();
    px[1] += evt->vp().y();

    // Convert position to mercator
    point2d<double> merc = utils::px2merc(evt->vp().z(), px);
    (*(m_selection.it)).lon = merc.x();
    (*(m_selection.it)).lat = merc.y();

    // Make sure item remains highlighted when mouse is released
    m_selection.highlight = false;

    // Trigger redraw
    notify_observers();

    return true;
}

bool gpxlayer::release(const layer_mouseevent* evt)
{
    // Button release on an existing item
    if (m_selection.it != m_trkpts.end()) 
    {
        // Toggle highlight status
        m_selection.highlight = !m_selection.highlight;
        notify_observers();
        return true;
    };

    // Add a new item

    // Viewport-relative to absolute map coordinate
    point2d<unsigned long> px(
            evt->pos().x() + evt->vp().x(),
            evt->pos().y() + evt->vp().y());

    // A new point is to be added
    // Try to convert the pixel position to mercator coordinate
    point2d<double> merc;
    try {
        merc = utils::px2merc(evt->vp().z(), px);
    } catch (...) {
        return false;
    }

    // Add the position to the list
    gpx_trkpt p;
    p.lon = merc.x();
    p.lat = merc.y();
    p.time = 0;
    p.ele = 0; 

    m_trkpts.push_back(p);

    // Select the newly added item
    m_selection.it = m_trkpts.end()-1;
    m_selection.highlight = true;

    // Indicate that this layer has changed
    notify_observers();

    return true;
}

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

bool gpxlayer::handle_evt_mouse(const layer_mouseevent* evt)
{
    // Only the left mouse button is of interest
    if (evt->button() != layer_mouseevent::BUTTON_LEFT)
        return false;

    int ret = false;
    switch (evt->action())
    {
        case layer_mouseevent::ACTION_PRESS:
        {
            ret = press(evt); 
            break;
        }
        case layer_mouseevent::ACTION_RELEASE:
        {
            ret = release(evt); 
            break;
        }
        case layer_mouseevent::ACTION_DRAG:
        {
            ret = drag(evt); 
            break;
        }
        default:
            ;
    }

    return ret;
}

bool gpxlayer::handle_evt_key(const layer_keyevent* evt)
{
    int ret = false;

    switch (evt->action())
    {
        case layer_keyevent::ACTION_PRESS:
        {
            ret = false; 
            break;
        }
        case layer_keyevent::ACTION_RELEASE:
        {
            ret = key(evt); 
            break;
        }
        default:
            ;
    }

    return ret;
}

void gpxlayer::draw(const viewport &vp, canvas &os)
{
    color color_track(0xff0000);
    color color_point(0x0000ff);
    color color_point_hl(0x00ff00);

    point2d<unsigned long> px_last;
    std::vector<gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin();it!=m_trkpts.end();++it) 
    {
        // Convert mercator to pixel coordinates
        point2d<unsigned long> px = utils::merc2px(vp.z(), point2d<double>((*it).lon, (*it).lat)); 
        px.x(px.x() - vp.x());
        px.y(px.y() - vp.y());

        // No connecting line possible if this is the first point
        if (it != m_trkpts.begin()) 
        {
            // Draw a connection between points
            os.fgcolor(color_track);
            os.line(px_last.x(), px_last.y(), px.x(), px.y(), 2);
        }

        // Draw crosshairs _above_ the connecting lines
        if ((m_trkpts.size() == 1) || (it == (m_trkpts.end()-1)))
        {
            if ((m_selection.it == it) && m_selection.highlight)
                os.fgcolor(color_point_hl);
            else
                os.fgcolor(color_point);

            os.line(px.x()-6, px.y(), px.x()+6, px.y(), 1);
            os.line(px.x(), px.y()-6, px.x(), px.y()+6, 1);
        }

        if (it != m_trkpts.begin())
        {
            if ((m_selection.it == (it-1)) && m_selection.highlight)
                os.fgcolor(color_point_hl);
            else
                os.fgcolor(color_point);

            os.line(px_last.x()-6, px_last.y(), px_last.x()+6, px_last.y(), 1);
            os.line(px_last.x(), px_last.y()-6, px_last.x(), px_last.y()+6, 1);
        }

        px_last = px;
    }

    trip();
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

