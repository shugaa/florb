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

bool gpxlayer::press(const layer_mouseevent* evt)
{
    // Mouse push outside viewport
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

    // New selection, highlight
    if ((m_selection.it != it))
        m_selection.highlight = true;
    // Same item as last time, toggle highlighting
    else
        m_selection.highlight = !m_selection.highlight;
    
    // Either we have a valid iterator position or end() which is later on used
    // to tell whether we are actually dragging something around or not
    m_selection.it = it;

    // Found an existing item. Save as it might be used in a dragging operation
    if (it != m_trkpts.end())
    {
        m_selection.trkpt = *it;

        // Trigger repaint (for highlighting / unhighlighting)
        notify_observers();
    }

    return true;
}

bool gpxlayer::drag(const layer_mouseevent* evt)
{
    // Viewport-relative to absolute coordinate
    point2d<unsigned long> px(
        evt->pos().x() + evt->vp().x(),
        evt->pos().y() + evt->vp().y());

    // Nothing to drag around
    if (m_selection.it == m_trkpts.end())
        return false;

    // Calculate the delta between the original and the current item position 
    point2d<unsigned long> cmp = utils::merc2px(
            evt->vp().z(), 
            point2d<double>(m_selection.trkpt.lon, m_selection.trkpt.lat)); 

    int dx = 0, dy = 0;
    if (px.x() >= cmp.x())
        dx = (px.x() - cmp.x());
    else
        dx = -(cmp.x() - px.x());
   
    if (px.y() >= cmp.y())
        dy = (px.y() - cmp.y());
    else
        dy = -(cmp.y() - px.y());

    // Add the delta and convert back to mercator
    point2d<double> merc = utils::px2merc(evt->vp().z(), point2d<unsigned long>(cmp.x()+dx, cmp.y()+dy));
    (*(m_selection.it)).lon = merc.x();
    (*(m_selection.it)).lat = merc.y();

    // Trigger redraw
    notify_observers();

    return true;
}

bool gpxlayer::release(const layer_mouseevent* evt)
{
    // Button release on an existing item
    if (m_selection.it != m_trkpts.end()) 
        return false;

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
    m_selection.trkpt = *(m_trkpts.end()-1);
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

