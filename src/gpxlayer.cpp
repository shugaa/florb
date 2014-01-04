#include <cmath>
#include <sstream>
#include <FL/x.H>
#include <FL/fl_draw.H>
#include "utils.hpp"
#include "settings.hpp"
#include "point.hpp"
#include "gpxlayer.hpp"

#define CLIPLEFT   (1)  // 0001
#define CLIPRIGHT  (2)  // 0010
#define CLIPBOTTOM (4)  // 0100
#define CLIPTOP    (8)  // 1000

gpxlayer::gpxlayer() :
    layer(),
    m_trip(0.0)
{
    m_selection.it = m_trkpts.end();

    name(std::string("Unnamed GPX layer"));
    register_event_handler<gpxlayer, layer::event_mouse>(this, &gpxlayer::handle_evt_mouse);
    register_event_handler<gpxlayer, layer::event_key>(this, &gpxlayer::handle_evt_key);
}

bool gpxlayer::key(const layer::event_key* evt)
{
    if (evt->key() != layer::event_key::KEY_DEL)
        return false;

    selection_delete();
    return true;
}

void gpxlayer::trip_update()
{
    if (m_trkpts.size() < 2) 
    {
        m_trip = 0.0;
        return;
    }

    point2d<double> p1((*(m_trkpts.end()-2)).lon, (*(m_trkpts.end()-2)).lat);
    point2d<double> p2((*(m_trkpts.end()-1)).lon, (*(m_trkpts.end()-1)).lat);
    point2d<double> g1(utils::merc2wsg84(p1));
    point2d<double> g2(utils::merc2wsg84(p2));
         
    m_trip += utils::dist(g1, g2);
}

void gpxlayer::trip_calcall()
{
    if (m_trkpts.size() < 2)
    {
        m_trip = 0.0;
        return;
    }

    double sum = 0.0;
    std::vector<gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin()+1;it!=m_trkpts.end();++it)
    {
        point2d<double> p1((*it).lon, (*it).lat);
        point2d<double> p2((*(it-1)).lon, (*(it-1)).lat);
        point2d<double> g1(utils::merc2wsg84(p1));
        point2d<double> g2(utils::merc2wsg84(p2));
         
        sum += utils::dist(g1, g2);
    }

    m_trip = sum;
}

bool gpxlayer::press(const layer::event_mouse* evt)
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

    m_selection.dragging = false;

    // New selection, turn highlighting off, it will be toggled on mouse button
    // release
    if ((m_selection.it != it))
        m_selection.highlight = false;
    
    // Either we have a valid iterator position or end() which is later on used
    // to tell whether we are actually dragging something around or not
    m_selection.it = it;

    return true;
}

bool gpxlayer::drag(const layer::event_mouse* evt)
{
    // Nothing to drag around
    if (m_selection.it == m_trkpts.end())
        return false;

    m_selection.dragging = true;

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
    notify();

    return true;
}

bool gpxlayer::release(const layer::event_mouse* evt)
{
    // Button release on an existing item
    if (m_selection.it != m_trkpts.end()) 
    {
        // Toggle highlight status
        m_selection.highlight = !m_selection.highlight;

        // Item has been dragged, recalculate trip
        if (m_selection.dragging)
            trip_calcall();

        notify();
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
    p.time = time(NULL);
    p.ele = 0.0; 
    m_trkpts.push_back(p);
    
    // Update current trip
    trip_update();

    // Select the newly added item
    m_selection.it = m_trkpts.end()-1;
    m_selection.highlight = true;

    // Indicate that this layer has changed
    notify();

    return true;
}

void gpxlayer::add_trackpoint(const point2d<double>& p)
{
    // Add the position to the list
    point2d<double> merc(utils::wsg842merc(p));

    gpx_trkpt ptrk;
    ptrk.lon = merc.x();
    ptrk.lat = merc.y();
    ptrk.time = time(NULL);
    ptrk.ele = 0.0; 
    m_trkpts.push_back(ptrk);
    
    // Update current trip
    trip_update();

    // Select the newly added item
    m_selection.it = m_trkpts.end()-1;
    m_selection.highlight = true;

    // Indicate that this layer has changed
    notify(); 
}

void gpxlayer::load_track(const std::string &path)
{
    name(path);
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path.c_str()) != tinyxml2::XML_NO_ERROR)
        throw 0;

    m_trkpts.clear();
    parsetree(doc.RootElement());
    notify();
};

void gpxlayer::save_track(const std::string &path)
{
    tinyxml2::XMLDocument doc;
    
    // XML standard declaration
    doc.NewDeclaration();
    
    tinyxml2::XMLElement *e1, *e2, *e3;
    tinyxml2::XMLText *t1;

    // Add a gpx element
    e1 = doc.NewElement("gpx");
    e1->SetAttribute("version",            "1.1");
    e1->SetAttribute("creator",            "florb");
    e1->SetAttribute("xmlns:xsi",          "http://www.w3.org/2001/XMLSchema-instance");
    e1->SetAttribute("xmlns",              "http://www.topografix.com/GPX/1/1");
    e1->SetAttribute("xsi:schemaLocation", "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd");
    doc.InsertEndChild(e1);

    // Add track
    e1 = e1->InsertEndChild(doc.NewElement("trk"))->ToElement();

    // Track name child
    e2 = doc.NewElement("name");
    t1 = doc.NewText(name().c_str());
    e2->InsertEndChild(t1);
    e1->InsertEndChild(e2);

    // Track number child
    e2 = doc.NewElement("number");
    t1 = doc.NewText("1");
    e2->InsertEndChild(t1);
    e1->InsertEndChild(e2);

    // Add a track segment child
    e1 = e1->InsertEndChild(doc.NewElement("trkseg"))->ToElement();

    // Add trackpoints to the segment
    std::vector<gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin();it!=m_trkpts.end();++it) 
    {
        // Trackpoint element
        point2d<double> wsg84(utils::merc2wsg84(point2d<double>((*it).lon, (*it).lat)));

        e2 = doc.NewElement("trkpt");
        e2->SetAttribute("lat", wsg84.y());
        e2->SetAttribute("lon", wsg84.x());
        e1->InsertEndChild(e2);

        // Elevation child
        std::ostringstream ss;
        ss.precision(6);
        ss.setf(std::ios::fixed, std::ios::floatfield);
        ss << (*it).ele;

        e3 = doc.NewElement("ele");
        t1 = doc.NewText(ss.str().c_str());
        e3->InsertEndChild(t1);
        e2->InsertEndChild(e3);

        // Time child
        e3 = doc.NewElement("time");
        t1 = doc.NewText(utils::timet2iso8601((*it).time).c_str());
        e3->InsertEndChild(t1);
        e2->InsertEndChild(e3);
    }

    doc.SaveFile(path.c_str()); 
}

void gpxlayer::clear_track()
{
   name(std::string("Unnamed GPX layer"));
   m_trkpts.clear();
   m_selection.it = m_trkpts.end();
   trip_calcall();
   notify();
}

double gpxlayer::trip()
{
    return m_trip;
}

void gpxlayer::notify()
{
    event_notify e;
    fire(&e);
}

bool gpxlayer::selected()
{
    return ((m_selection.it != m_trkpts.end()) && (m_selection.highlight));
}

point2d<double> gpxlayer::selection_pos()
{
    if (!selected())
        throw 0;

    double lonmerc = (*(m_selection.it)).lon;
    double latmerc = (*(m_selection.it)).lat;

    return utils::merc2wsg84(point2d<double>(lonmerc, latmerc));
}

void gpxlayer::selection_pos(const point2d<double>& p)
{
    if (!selected())
        throw 0;

    point2d<double> pmerc(utils::wsg842merc(p));
    
    (*(m_selection.it)).lon = pmerc.x();
    (*(m_selection.it)).lat = pmerc.y();
    notify();
}

double gpxlayer::selection_elevation()
{
    if (!selected())
        throw 0;

    return (*(m_selection.it)).ele;
}

void gpxlayer::selection_elevation(double e)
{
    if (!selected())
        throw 0;

    (*(m_selection.it)).ele = e;    
    notify();
}

void gpxlayer::selection_delete()
{
    if (!selected())
        throw 0;

    m_trkpts.erase(m_selection.it);
    m_selection.it = m_trkpts.end();

    // Recalculate trip for the entire track
    trip_calcall();

    notify();
}

gpxlayer::~gpxlayer()
{
    ;
};

bool gpxlayer::handle_evt_mouse(const layer::event_mouse* evt)
{
    // Only the left mouse button is of interest
    if (evt->button() != layer::event_mouse::BUTTON_LEFT)
        return false;

    int ret = false;
    switch (evt->action())
    {
        case layer::event_mouse::ACTION_PRESS:
        {
            ret = press(evt); 
            break;
        }
        case layer::event_mouse::ACTION_RELEASE:
        {
            ret = release(evt); 
            break;
        }
        case layer::event_mouse::ACTION_DRAG:
        {
            ret = drag(evt); 
            break;
        }
        default:
            ;
    }

    return ret;
}

bool gpxlayer::handle_evt_key(const layer::event_key* evt)
{
    int ret = false;

    switch (evt->action())
    {
        case layer::event_key::ACTION_PRESS:
        {
            ret = false; 
            break;
        }
        case layer::event_key::ACTION_RELEASE:
        {
            ret = key(evt); 
            break;
        }
        default:
            ;
    }

    return ret;
}

// Cohen–Sutherland clipping algorithm
bool gpxlayer::clipline(point2d<double> &p1, point2d<double> &p2, point2d<double> r1, point2d<double> r2, bool &p1clip, bool &p2clip)
{
    bool ret = false;
    double xmin, xmax, ymin, ymax;

    if (r1.x() > r2.x())
    {
        xmin = r2.x();
        xmax = r1.x();
    }
    else
    {
        xmin = r1.x();
        xmax = r2.x();
    }

    if (r1.y() > r2.y())
    {
        ymin = r2.y();
        ymax = r1.y();
    }
    else
    {
        ymin = r1.y();
        ymax = r2.y();
    } 

    // Calculate m and n for this line
    double m = (p2.y() - p1.y()) / (p2.x() - p1.x());
    double n = p1.y() - m * p1.x();

    // Max 2 clipping operations per point and one last check operation. The
    // number of iterations is limited because for very large m and n the
    // datatype might overflow and create an endless loop. In this case the
    // loop exits and the line is simply not drawn.
    for (int i=0;i<5;i++)
    {
        // Compute code for both points
        int code1 = 0, code2 = 0;
        
        if (p1.x() < xmin)
            code1 |= CLIPLEFT;
        if (p1.x() > xmax)
            code1 |= CLIPRIGHT;
        if (p1.y() < ymin)
            code1 |= CLIPTOP;
        if (p1.y() > ymax)
            code1 |= CLIPBOTTOM;

        if (p2.x() < xmin)
            code2 |= CLIPLEFT;
        if (p2.x() > xmax)
            code2 |= CLIPRIGHT;
        if (p2.y() < ymin)
            code2 |= CLIPTOP;
        if (p2.y() > ymax)
            code2 |= CLIPBOTTOM;

        // Both inside, draw line
        if ((code1 | code2) == 0)
        {
            ret = true;
            break;
        }

        // Both top, bottom, left or right outside, line need not be drawn
        if ((code1 & code2) != 0)
        {
            ret = false;
            break;
        }

        // Pick an endpoint for clipping
        point2d<double> &ptmp = (code1 != 0) ? p1 : p2;
        int codetmp;
        if (code1 != 0)
        {
            codetmp = code1;
            p1clip = true;
        }
        else
        {
            codetmp = code2;
            p2clip = true;
        }

        // Clip top
        if (codetmp & CLIPTOP)
        {
            ptmp[0] = (ymin - n) / m; 
            ptmp[1] = ymin;
        }
        // Clip Bottom
        else if (codetmp & CLIPBOTTOM)
        {
            ptmp[0] = (ymax - n) / m; 
            ptmp[1] = ymax;
        }
        // Clip Left
        else if (codetmp & CLIPLEFT)
        {
            ptmp[0] = xmin; 
            ptmp[1] = m * xmin + n;
        }
        // Clip Right
        else if (codetmp & CLIPRIGHT)
        {
            ptmp[0] = xmax; 
            ptmp[1] = m * xmax + n;
        }
    }

    return ret;
}

void gpxlayer::draw(const viewport &vp, canvas &os)
{
    color color_track(0xff0000);
    color color_point(0x0000ff);
    color color_point_hl(0x00ff00);

    point2d<double> pmerc_last;
    point2d<double> pmerc_r1(utils::px2merc(vp.z(), point2d<unsigned long>(vp.x(), vp.y())));
    point2d<double> pmerc_r2(utils::px2merc(vp.z(), point2d<unsigned long>(vp.x()+vp.w()-1, vp.y()+vp.h()-1)));

    std::vector<gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin();it!=m_trkpts.end();++it) 
    {
        point2d<double> pmerc((*it).lon, (*it).lat);
        point2d<unsigned long> ppx;
        point2d<unsigned long> ppx_last;

        bool curclip = false;
        bool lastclip = false;

        // No connecting line possible if this is the first point
        if (it != m_trkpts.begin()) 
        {
            if (pmerc == pmerc_last)
                continue;

            bool dodraw = clipline(pmerc_last, pmerc, pmerc_r1, pmerc_r2, lastclip, curclip);

            ppx = utils::merc2px(vp.z(), pmerc);
            ppx_last = utils::merc2px(vp.z(), pmerc_last);

            // Draw a connection between points
            if (dodraw)
            {
                ppx[0] -= vp.x();
                ppx[1] -= vp.y();
                ppx_last[0] -= vp.x();
                ppx_last[1] -= vp.y();

                os.fgcolor(color_track);
                os.line(ppx_last.x(), ppx_last.y(), ppx.x(), ppx.y(), 2);
            }
            else
            {
                // Both points outside, nothing to do
                pmerc_last = point2d<double>((*it).lon, (*it).lat);
                continue;
            }
        } else
        {
            ppx = utils::merc2px(vp.z(), pmerc);
            ppx[0] -= vp.x();
            ppx[1] -= vp.y();
        }

        // Draw crosshairs _above_ the connecting lines
#if 1
        if (((m_trkpts.size() == 1) || (it == (m_trkpts.end()-1))) && (!curclip))
        {
            if ((m_selection.it == it) && m_selection.highlight)
                os.fgcolor(color_point_hl);
            else
                os.fgcolor(color_point);

            os.line(ppx.x()-6, ppx.y(), ppx.x()+6, ppx.y(), 1);
            os.line(ppx.x(), ppx.y()-6, ppx.x(), ppx.y()+6, 1);
        }

        if ((it != m_trkpts.begin()) && (!lastclip))
        {
            if ((m_selection.it == (it-1)) && m_selection.highlight)
                os.fgcolor(color_point_hl);
            else
                os.fgcolor(color_point);

            os.line(ppx_last.x()-6, ppx_last.y(), ppx_last.x()+6, ppx_last.y(), 1);
            os.line(ppx_last.x(), ppx_last.y()-6, ppx_last.x(), ppx_last.y()+6, 1);
        }
#endif

        pmerc_last = point2d<double>((*it).lon, (*it).lat);
    }
}

void gpxlayer::parsetree(tinyxml2::XMLNode *parent)
{
    bool ret = true;
    tinyxml2::XMLElement *etmp = parent->ToElement();

    for (;;)
    {
        // Not an XML element by something else
        if (etmp == NULL) 
        {
            break;
        }

        gpx_trkpt p;
        std::string val(parent->Value());

        // Handle trackpoint
        if (val.compare("trkpt") == 0) {
            double lat = 1234.5, lon = 1234.5;
            etmp->QueryDoubleAttribute("lat", &lat);
            etmp->QueryDoubleAttribute("lon", &lon);

            // Check for error
            if ((lat == 1234.5) || (lon == 1234.5))
            {
                ret = false;
                break;
            }

            // Convert to mercator coordinates
            point2d<double> merc(utils::wsg842merc(point2d<double>(lon, lat)));
            p.lon = merc.x();
            p.lat = merc.y();
            p.time = 0;
            p.ele = 0;

            // Look for "time" and "ele" childnodes
            tinyxml2::XMLNode *child;
            for (child = parent->FirstChild(); child != NULL; child = child->NextSibling()) {
                if (std::string(child->Value()).compare("time") == 0) {
                    p.time = utils::iso8601_2timet(std::string(child->ToElement()->GetText()));
                }
                else if (std::string(child->Value()).compare("ele") == 0) {
                    std::istringstream iss(child->ToElement()->GetText());
                    iss >> p.ele;
                }
            }

            // Add the point to the list and update the trip counter
            m_trkpts.push_back(p);
            trip_update();
        }
        // Handle trackname element
        else if (val.compare("name") == 0) 
        {
            name(std::string(etmp->GetText()));
        }
        // TODO: Track number anyone?
        else if (val.compare("number") == 0) 
        {
            ;
        }

        break;
    }

    if (ret == false)
        throw("XML parser error");

    // Recurse the rest of the subtree
    tinyxml2::XMLNode *child;
    for (child = parent->FirstChild(); child != NULL; child = child->NextSibling()) {
        parsetree(child);
    }
}

