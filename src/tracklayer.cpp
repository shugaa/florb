#include <cmath>
#include <sstream>
#include <algorithm>
#include <FL/x.H>
#include <FL/fl_draw.H>
#include <clocale>
#include "utils.hpp"
#include "version.hpp"
#include "settings.hpp"
#include "point.hpp"
#include "tracklayer.hpp"

const std::string florb::tracklayer::trackname = "New GPX track";

florb::tracklayer::tracklayer() :
    layer(),
    m_trip(0.0)
{
    name(std::string(_(trackname.c_str())));
    register_event_handler<florb::tracklayer, florb::layer::event_mouse>(this, &florb::tracklayer::handle_evt_mouse);
    register_event_handler<florb::tracklayer, florb::layer::event_key>(this, &florb::tracklayer::handle_evt_key);
}

void florb::tracklayer::trip_update()
{
    // Less than 2 trackpoints, can't calculate trip
    if (m_trkpts.size() < 2) 
    {
        m_trip = 0.0;
        return;
    }

    // Take the last trackpoint in the list and the one before that and
    // calculate the distance. Then add to trip.
    florb::point2d<double> p1((*(m_trkpts.end()-2)).lon, (*(m_trkpts.end()-2)).lat);
    florb::point2d<double> p2((*(m_trkpts.end()-1)).lon, (*(m_trkpts.end()-1)).lat);
    m_trip += florb::utils::dist(florb::utils::merc2wsg84(p1), florb::utils::merc2wsg84(p2));
}

void florb::tracklayer::trip_calcall()
{
    // Less than 2 trackpoints, can't calculate trip
    if (m_trkpts.size() < 2)
    {
        m_trip = 0.0;
        return;
    }

    // Add up the distances between any consecutive trackpoints in the list
    m_trip = 0.0;
    std::vector<florb::tracklayer::gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin()+1;it!=m_trkpts.end();++it)
    {
        florb::point2d<double> p1((*it).lon, (*it).lat);
        florb::point2d<double> p2((*(it-1)).lon, (*(it-1)).lat);
        m_trip += florb::utils::dist(florb::utils::merc2wsg84(p1), florb::utils::merc2wsg84(p2));
    }
}

bool florb::tracklayer::key(const florb::layer::event_key* evt)
{
    // We only care for the DEL key at the moment
    if (evt->key() != florb::layer::event_key::KEY_DEL)
        return false;

    selection_delete();
    return true;
}

bool florb::tracklayer::press(const florb::layer::event_mouse* evt)
{
    florb::point2d<unsigned long> pxabs;
    pxabs[0] = evt->pos().x() < 0 ? 0 : (unsigned long)evt->pos().x();
    pxabs[1] = evt->pos().y() < 0 ? 0 : (unsigned long)evt->pos().y();

    // Mouse push outside viewport area
    if (pxabs.x() >= evt->vp().w())
        pxabs[0] = evt->vp().w()-1;
    if (pxabs.y() >= evt->vp().h())
        pxabs[1] = evt->vp().h()-1;

    // Convert to absolute map coordinate
    pxabs.x(pxabs.x() + evt->vp().x());
    pxabs.y(pxabs.y() + evt->vp().y());

    // Clear current selection
    m_selection.waypoints.clear();

    // Find an existing item for this mouse position
    std::vector<florb::tracklayer::gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin();it!=m_trkpts.end();++it)
    {
        florb::point2d<unsigned long> cmp = florb::utils::merc2px(evt->vp().z(), florb::point2d<double>((*it).lon, (*it).lat)); 

        // Check whether the click might refer to this point
        if (pxabs.x() >= (cmp.x()+wp_hotspot))
            continue;
        if (pxabs.x() < ((cmp.x()>=wp_hotspot) ? cmp.x()-wp_hotspot : 0))
            continue;
        if (pxabs.y() >= (cmp.y()+wp_hotspot))
            continue;
        if (pxabs.y() < ((cmp.y()>=wp_hotspot) ? cmp.y()-wp_hotspot : 0))
            continue;

        break;
    }

    // Not dragging (yet)
    m_selection.dragging = false;
    m_selection.multiselect = false;
    m_selection.dragorigin = florb::utils::px2merc(evt->vp().z(), pxabs);

    // New selection
    if (it != m_trkpts.end())
    {
        m_selection.waypoints.push_back(it);
        notify();
    }

    return true;
}

bool florb::tracklayer::drag(const florb::layer::event_mouse* evt)
{
    // Enter drag mode
    m_selection.dragging = true;

    florb::point2d<unsigned long> px(evt->pos().x(), evt->pos().y());
    
    // Catch drag outside map area
    if (evt->pos().x() < 0)
        px[0] = 0;
    else if (evt->pos().x() >= (int)evt->vp().w())
        px[0] = evt->vp().w()-1;
    if (evt->pos().y() < 0)
        px[1] = 0;
    else if (evt->pos().y() >= (int)evt->vp().h())
        px[1] = evt->vp().h()-1;

    // Convert viewport relative to map coordinate
    px[0] += evt->vp().x();
    px[1] += evt->vp().y();

    // Convert map coordinate to mercator
    florb::point2d<double> merc = florb::utils::px2merc(evt->vp().z(), px);

    // Dragging a single active waypoint around
    if ((m_selection.waypoints.size() == 1) && (m_selection.multiselect == false))
    { 
        // Update the position for the trackpoint currently being dragged
        (*(m_selection.waypoints[0])).lon = merc.x();
        (*(m_selection.waypoints[0])).lat = merc.y();
    }
    // Selecting multiple waypoints
    else
    {
        // Update current drag coordinate for later reference
        m_selection.dragcurrent = merc;
        m_selection.multiselect = true;

        // Clear the list of selected waypoints
        m_selection.waypoints.clear();

        // Check for each waypoint whether it is within the selection rectangle
        double top, bottom, left, right;
        if (m_selection.dragorigin.y() < m_selection.dragcurrent.y())
        {
            top = m_selection.dragorigin.y();
            bottom = m_selection.dragcurrent.y();
        }
        else
        {
            top = m_selection.dragcurrent.y();
            bottom = m_selection.dragorigin.y(); 
        }

        if (m_selection.dragorigin.x() < m_selection.dragcurrent.x())
        {
            left = m_selection.dragorigin.x();
            right = m_selection.dragcurrent.x();
        }
        else
        {
            left = m_selection.dragcurrent.x();
            right = m_selection.dragorigin.x(); 
        }

        std::vector<florb::tracklayer::gpx_trkpt>::iterator it;
        for (it=m_trkpts.begin();it!=m_trkpts.end();++it)
        {
            florb::point2d<double> cmp((*it).lon, (*it).lat); 

            // Check whether the point is inside the selection rectangle
            if (cmp.x() < left)
                continue;
            if (cmp.x() > right)
                continue;
            if (cmp.y() < top)
                continue;
            if (cmp.y() > bottom)
                continue;

            // Trackpoint inside rectangle, add to list
            m_selection.waypoints.push_back(it);
        }
    }

    // Trigger redraw
    notify();

    return true;
}

bool florb::tracklayer::release(const florb::layer::event_mouse* evt)
{
    // Button release on an existing item
    if ((m_selection.waypoints.size() == 1) && (m_selection.multiselect == false))
    {
        // Item has been dragged, recalculate trip
        if (m_selection.dragging)
            trip_calcall();

        // Update
        notify();
        return true;
    };

    // Button release for multiple selection
    if (m_selection.multiselect)
    {
        m_selection.multiselect = false;
    }

    // Do not add a new waypoint, this is the end of a drag operation
    if (m_selection.dragging)
    {
        notify();
        return true;
    }

    // Not exiting from drag mode, add a new waypoint
    // Viewport-relative to absolute map coordinate
    florb::point2d<unsigned long> px(
            evt->pos().x() + evt->vp().x(),
            evt->pos().y() + evt->vp().y());

    // Try to convert the pixel position to wsg84
    florb::point2d<double> wsg84;
    try {
        wsg84 = florb::utils::px2wsg84(evt->vp().z(), px);
    } catch (...) {
        return false;
    }

    // Add the new trackpoint
    add_trackpoint(wsg84);

    return true;
}

void florb::tracklayer::add_trackpoint(const florb::point2d<double>& p)
{
    // Add the position to the list
    florb::point2d<double> merc(florb::utils::wsg842merc(p));

    florb::tracklayer::gpx_trkpt ptrk;
    ptrk.lon = merc.x();
    ptrk.lat = merc.y();
    ptrk.time = time(NULL);
    ptrk.ele = 0.0; 
    m_trkpts.push_back(ptrk);
    
    // Update current trip
    trip_update();

    // Select the newly added item
    m_selection.waypoints.clear();
    m_selection.waypoints.push_back(m_trkpts.end()-1);

    // Indicate that this layer has changed
    notify(); 
}

void florb::tracklayer::load_track(const std::string &path)
{
    // Load the XML
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS)
        throw std::runtime_error(_("Failed to open GPX file"));

    // Clear existing track and selection
    m_trkpts.clear();
    m_selection.waypoints.clear();

    // TinyXML's number parsing is locale dependent, so we switch to "C"
    // and back after parsing
    char *poldlc;
    char oldlc[16];
    poldlc = setlocale(LC_ALL, NULL);
    strncpy(oldlc, poldlc, 15);
    oldlc[15] = '\0';
    setlocale(LC_ALL, "C");

    // Parse the GPX XML
    tinyxml2::XMLElement* root = doc.RootElement();
    if (root)
        parsetree(doc.RootElement());
    else
    {
        // Switch back to original locale before throwing
        setlocale(LC_ALL, oldlc); 
        throw std::runtime_error(_("Failed to open GPX file"));
    }

    // Switch back to original locale
    setlocale(LC_ALL, oldlc); 

    // The filename will be the name for the loaded track
    name(florb::utils::filestem(path));

    // Request update
    notify();
};

void florb::tracklayer::save_track(const std::string &path)
{
    // TinyXML's number parsing is locale dependent, set to "C" and restore
    // later
    char *poldlc;
    char oldlc[16];
    poldlc = setlocale(LC_ALL, NULL);
    strncpy(oldlc, poldlc, 15);
    oldlc[15] = '\0';
    setlocale(LC_ALL, "C");

    // Create an XML document
    tinyxml2::XMLDocument doc;
    
    // XML standard declaration
    doc.NewDeclaration();
    
    // 3 Levels of elements and children
    tinyxml2::XMLElement *e1, *e2, *e3;
    tinyxml2::XMLText *t1;

    // Add a gpx element
    e1 = doc.NewElement("gpx");
    e1->SetAttribute("version",            "1.1");
    e1->SetAttribute("creator",            FLORB_PROGSTR);
    e1->SetAttribute("xmlns:xsi",          "http://www.w3.org/2001/XMLSchema-instance");
    e1->SetAttribute("xmlns",              "http://www.topografix.com/GPX/1/1");
    e1->SetAttribute("xsi:schemaLocation", "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd");
    doc.InsertEndChild(e1);

    // Add track
    e1 = e1->InsertEndChild(doc.NewElement("trk"))->ToElement();

    // Track name child
    e2 = doc.NewElement("name");
    t1 = doc.NewText(florb::utils::filestem(path).c_str());
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
    std::vector<florb::tracklayer::gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin();it!=m_trkpts.end();++it) 
    {
        // Trackpoint element
        florb::point2d<double> wsg84(florb::utils::merc2wsg84(florb::point2d<double>((*it).lon, (*it).lat)));

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
        t1 = doc.NewText(florb::utils::timet2iso8601((*it).time).c_str());
        e3->InsertEndChild(t1);
        e2->InsertEndChild(e3);
    }

    // Try to save and restore original locale
#if (TIXML2_MAJOR_VERSION < 2)
    // My Ubuntu / Mint has a really weird tinyxml2 prerelease
    int xmlerr = doc.SaveFile(path.c_str());
#else
    tinyxml2::XMLError xmlerr = doc.SaveFile(path.c_str());
#endif

    setlocale(LC_ALL, oldlc);

    // Throw in case of error
    if (xmlerr !=  tinyxml2::XML_SUCCESS)
        throw std::runtime_error(_("Failed to save GPX data"));
}

void florb::tracklayer::clear_track()
{
   // Reset the track name
   name(std::string(_(trackname.c_str())));

   // Clear the list of trackpoints and the current selection
   m_trkpts.clear();
   m_selection.waypoints.clear();

   // Recalculate the trip (well, this will turn out to be 0.0)
   trip_calcall();

   // Request update
   notify();
}

double florb::tracklayer::trip()
{
    return m_trip;
}

void florb::tracklayer::showwpmarkers(bool s)
{
    // Set flag and request update
    m_showwpmarkers = s;
    notify();
}

void florb::tracklayer::notify()
{
    event_notify e;
    fire(&e);
}

size_t florb::tracklayer::selected()
{
    // Return the numer of selected waypoints
    return m_selection.waypoints.size();
}

void florb::tracklayer::selection_get(std::vector<waypoint>& waypoints)
{
    // Clear the passed vector
    waypoints.clear();

    // Return a list of selected waypoints
    std::vector< std::vector<florb::tracklayer::gpx_trkpt>::iterator >::iterator it;
    for(it=m_selection.waypoints.begin();it!=m_selection.waypoints.end();++it)
    {
        florb::point2d<double> pwsg84(florb::utils::merc2wsg84(florb::point2d<double>((*(*it)).lon, (*(*it)).lat)));

        waypoint tmp(pwsg84.x(), pwsg84.y(), (*(*it)).ele, (*(*it)).time);
        waypoints.push_back(tmp);
    }
}

void florb::tracklayer::selection_set(const std::vector<waypoint>& waypoints)
{
    // Number of waypoints must be the same
    if (waypoints.size() != m_selection.waypoints.size())
        throw std::runtime_error(_("Error updating selected waypoints"));;

    // Update the internal representation of each selected waypoint
    std::vector< std::vector<florb::tracklayer::gpx_trkpt>::iterator >::iterator it;
    size_t i;
    for(it=m_selection.waypoints.begin(), i=0;it!=m_selection.waypoints.end();++it,i++)
    {
        florb::point2d<double> pmerc(florb::utils::wsg842merc(florb::point2d<double>(waypoints[i].lon(), waypoints[i].lat())));

        (*(*it)).lon = pmerc.x();
        (*(*it)).lat = pmerc.y();
        (*(*it)).ele = waypoints[i].elevation();
        (*(*it)).time = waypoints[i].time();
    }
}

void florb::tracklayer::selection_delete()
{
    // No selection or no waypoints
    if ((selected() == 0) || (m_trkpts.size() == 0))
        throw std::out_of_range("Invalid selection"); 

    // Delete all selected waypoints, back to front
    std::vector< std::vector<florb::tracklayer::gpx_trkpt>::iterator >::iterator it = m_selection.waypoints.end();
    do
    {
        --it;
        m_trkpts.erase(*it);

    } while (it != m_selection.waypoints.begin());

    // Clear the list of selected waypoints
    m_selection.waypoints.clear();

    // If there are waypoints remaining, select the last one in the list
    if (m_trkpts.size() > 0)
        m_selection.waypoints.push_back(m_trkpts.end()-1);

    // Recalculate trip for the entire track and request update
    trip_calcall();
    notify();
}

florb::tracklayer::~tracklayer()
{
    ;
};

bool florb::tracklayer::handle_evt_mouse(const florb::layer::event_mouse* evt)
{
    // Only the left mouse button is of interest
    if (!enabled())
        return false;
    if (evt->button() != florb::layer::event_mouse::BUTTON_LEFT)
        return false;

    bool ret = false;
    switch (evt->action())
    {
        case florb::layer::event_mouse::ACTION_PRESS:
        {
            ret = press(evt); 
            break;
        }
        case florb::layer::event_mouse::ACTION_RELEASE:
        {
            ret = release(evt); 
            break;
        }
        case florb::layer::event_mouse::ACTION_DRAG:
        {
            ret = drag(evt); 
            break;
        }
        default:
            ;
    }

    return ret;
}

bool florb::tracklayer::handle_evt_key(const florb::layer::event_key* evt)
{   
    if (!enabled())
        return false;

    int ret = false;

    switch (evt->action())
    {
        case florb::layer::event_key::ACTION_PRESS:
        {
            ret = false; 
            break;
        }
        case florb::layer::event_key::ACTION_RELEASE:
        {
            ret = key(evt); 
            break;
        }
        default:
            ;
    }

    return ret;
}

bool florb::tracklayer::draw(const viewport &vp, florb::canvas &os)
{
    if (m_trkpts.size() == 0)
        return true;

    // TODO: Performance killer!!
    florb::cfg_ui cfgui = florb::settings::get_instance()["ui"].as<florb::cfg_ui>(); 

    florb::color color_track(cfgui.trackcolor());
    florb::color color_point(cfgui.markercolor());
    florb::color color_point_hl(cfgui.markercolorselected());
    florb::color color_selector(cfgui.selectioncolor());
    unsigned int linewidth = cfgui.tracklinewidth();

    florb::point2d<double> pmerc_last;
    florb::point2d<double> pmerc_r1(florb::utils::px2merc(vp.z(), florb::point2d<unsigned long>(vp.x(), vp.y())));
    florb::point2d<double> pmerc_r2(florb::utils::px2merc(vp.z(), florb::point2d<unsigned long>(vp.x()+vp.w()-1, vp.y()+vp.h()-1)));

    std::vector<florb::tracklayer::gpx_trkpt>::iterator it;
    for (it=m_trkpts.begin();it!=m_trkpts.end();++it) 
    {
        florb::point2d<double> pmerc((*it).lon, (*it).lat);
        florb::point2d<unsigned long> ppx;
        florb::point2d<unsigned long> ppx_last;

        bool curclip = false;
        bool lastclip = false;

        // No connecting line possible if this is the first point
        if (it != m_trkpts.begin()) 
        {
            if (pmerc == pmerc_last)
                continue;

            bool dodraw = florb::utils::clipline(pmerc_last, pmerc, pmerc_r1, pmerc_r2, lastclip, curclip);

            ppx = florb::utils::merc2px(vp.z(), pmerc);
            ppx_last = florb::utils::merc2px(vp.z(), pmerc_last);

            // Draw a connection between points
            if (dodraw)
            {
                ppx[0] -= vp.x();
                ppx[1] -= vp.y();
                ppx_last[0] -= vp.x();
                ppx_last[1] -= vp.y();

                os.fgcolor(color_track);
                os.line(ppx_last.x(), ppx_last.y(), ppx.x(), ppx.y(), linewidth);
            }
            else
            {
                // Both points outside, nothing to do
                pmerc_last = florb::point2d<double>((*it).lon, (*it).lat);
                continue;
            }
        } else
        {
            ppx = florb::utils::merc2px(vp.z(), pmerc);
            ppx[0] -= vp.x();
            ppx[1] -= vp.y();
        }

        // Draw crosshairs _above_ the connecting lines if requested
        if (m_showwpmarkers) 
        {
            if (((m_trkpts.size() == 1) || (it == (m_trkpts.end()-1))) && (!curclip))
            {
                if (std::find(m_selection.waypoints.begin(), m_selection.waypoints.end(), it) != m_selection.waypoints.end())
                    os.fgcolor(color_point_hl);
                else
                    os.fgcolor(color_point);

                os.line(ppx.x()-6, ppx.y(), ppx.x()+6, ppx.y(), 1);
                os.line(ppx.x(), ppx.y()-6, ppx.x(), ppx.y()+6, 1);
            }

            if ((it != m_trkpts.begin()) && (!lastclip))
            {
                if (std::find(m_selection.waypoints.begin(), m_selection.waypoints.end(), (it-1)) != m_selection.waypoints.end())
                    os.fgcolor(color_point_hl);
                else
                    os.fgcolor(color_point);

                os.line(ppx_last.x()-6, ppx_last.y(), ppx_last.x()+6, ppx_last.y(), 1);
                os.line(ppx_last.x(), ppx_last.y()-6, ppx_last.x(), ppx_last.y()+6, 1);
            }
        }

        pmerc_last = florb::point2d<double>((*it).lon, (*it).lat);
    }

    // Draw selection rectangle
    if (m_selection.multiselect)
    {
        florb::point2d<unsigned long> ppx_origin = florb::utils::merc2px(vp.z(), m_selection.dragorigin);
        florb::point2d<unsigned long> ppx_current = florb::utils::merc2px(vp.z(), m_selection.dragcurrent);

        ppx_origin[0] -= vp.x();
        ppx_origin[1] -= vp.y();
        ppx_current[0] -= vp.x();
        ppx_current[1] -= vp.y();

        os.fgcolor(color_selector);
        os.line(ppx_origin.x(), ppx_origin.y(), ppx_origin.x(), ppx_current.y(), 1);
        os.line(ppx_origin.x(), ppx_origin.y(), ppx_current.x(), ppx_origin.y(), 1);
        os.line(ppx_current.x(), ppx_current.y(), ppx_current.x(), ppx_origin.y(), 1);
        os.line(ppx_current.x(), ppx_current.y(), ppx_origin.x(), ppx_current.y(), 1);
    }

    return true;
}

void florb::tracklayer::parsetree(tinyxml2::XMLNode *parent)
{
    bool ret = true;
    tinyxml2::XMLElement *etmp = parent->ToElement();

    for (;;)
    {
        // Not an XML element but something else
        if (etmp == NULL) 
        {
            break;
        }

        florb::tracklayer::gpx_trkpt p;
        std::string val(parent->Value());

        // Handle trackpoint
        if ((val.compare("trkpt") == 0) || (val.compare("wpt") == 0))
        {
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
            florb::point2d<double> merc(florb::utils::wsg842merc(florb::point2d<double>(lon, lat)));
            p.lon = merc.x();
            p.lat = merc.y();
            p.time = 0;
            p.ele = 0;

            // Look for "time" and "ele" childnodes
            tinyxml2::XMLNode *child;
            for (child = parent->FirstChild(); child != NULL; child = child->NextSibling()) {
                if (std::string(child->Value()).compare("time") == 0) {
                    p.time = florb::utils::iso8601_2timet(std::string(child->ToElement()->GetText()));
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

    if (!ret)
        std::runtime_error("GPX XML parser error");

    // Recurse the rest of the subtree
    tinyxml2::XMLNode *child;
    for (child = parent->FirstChild(); child != NULL; child = child->NextSibling()) {
        parsetree(child);
    }
}

