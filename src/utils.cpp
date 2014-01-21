#include <cmath>
#include <stdexcept>
#include <iostream>
#include <boost/filesystem.hpp>
#include <sstream>
#include <cstdlib>
#include <X11/xpm.h>
#include <FL/x.H>
#include "utils.hpp"
#include "florb.xpm"

point2d<double> utils::wsg842merc(const point2d<double> &wsg84)
{
    if ((wsg84.x() > 180.0) || (wsg84.x() < -180.0))
        throw std::out_of_range("Invalid longitude");
    if ((wsg84.y() > 90.0) || (wsg84.y() < -90.0))
        throw std::out_of_range("Invalid latitude");

    // Anything above 85 degrees N or S is infinity in the mercator projection,
    // clip here.
    double lat = wsg84.y();
    if (lat > 85.0)
        lat = 85.0;
    else if (lat < -85)
        lat = -85.0;

    // Project to Mercator (360 by 360 square)
    return point2d<double>(
            180.0 + wsg84.x(),
            180.0 - ((180.0/M_PI) * log(tan(M_PI/4.0+lat*(M_PI/180.0)/2.0))));
}

point2d<unsigned long> utils::merc2px(unsigned int z, const point2d<double> &merc)
{
    unsigned long dimxy = dim(z);

    return point2d<unsigned long>(
        (unsigned long)(((double)(dimxy-1)/360.0) * merc.x()),
        (unsigned long)(((double)(dimxy-1)/360.0) * merc.y()));
}

point2d<unsigned long> utils::wsg842px(unsigned int z, const point2d<double> &wsg84)
{
    // Mercator projection
    point2d<double> merc(wsg842merc(wsg84));

    // Unit conversion
    return merc2px(z, merc);
}

point2d<double> utils::px2wsg84(unsigned int z, const point2d<unsigned long> &px)
{
    // Get map dimensions
    unsigned long dimxy = dim(z);

    // Make sure the coordinate is on the map
    if ((px.x() > dimxy) || (px.y() > dimxy))
        throw std::out_of_range("Invalid pixel position");

    // Unit conversion
    point2d<double> mdeg(
            (360.0/((double)dimxy/(double)px.x())),
            (360.0/((double)dimxy/(double)(dimxy-px.y()))));

    // Convert mercator to GPS coordinate
    return merc2wsg84(mdeg);
}

point2d<double> utils::merc2wsg84(const point2d<double>& wsg84)
{
    return point2d<double>( 
            wsg84.x() - 180.0,
            -((180.0/M_PI) * (2.0 * atan(exp((wsg84.y()-180.0)*M_PI/180.0)) - M_PI/2.0)));
}

point2d<double> utils::px2merc(unsigned int z, const point2d<unsigned long> &px)
{
    unsigned long dimxy = dim(z);

    // Make sure the coordinate is on the map
    if ((px.x() >= dimxy) || (px.y() >= dimxy))
    {
        throw std::out_of_range("Invalid pixel position");
    }

    double x = (px.x() == 0) ? 0.0 : (360.0/((double)(dimxy-1)/(double)px.x()));
    double y = (px.x() == 0) ? 0.0 : (360.0/((double)(dimxy-1)/(double)px.y()));
    return point2d<double>(x,y);
}

unsigned long utils::dim(unsigned int z)
{
    return pow(2.0, z) * 256;
}

double utils::dist(const point2d<double> &p1, const point2d<double> &p2)
{
    if (p1 == p2)
        return 0.0;

    double d2r = (M_PI/180.0);

    double lon1 = p1.x()*d2r;
    double lat1 = p1.y()*d2r;
    double lon2 = p2.x()*d2r;
    double lat2 = p2.y()*d2r;

    return (6378.388 * acos(sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(lon2 - lon1)));
}

double utils::dist_merc(const point2d<double> &p1, const point2d<double> &p2)
{
    if (p1 == p2)
        return 0.0;

    double circ = 2*M_PI*6372.7982;
    double dstmerc = sqrt( pow(std::abs(p1.x()-p2.x()), 2.0) + pow(std::abs(p1.y()-p2.y()), 2.0)*0.9444444 );
    return (dstmerc * (circ/360.0));
}

time_t utils::iso8601_2timet(const std::string& iso)
{
    struct tm stm;
    strptime(iso.c_str(), "%FT%T%z", &stm);

    return mktime(&stm);
}

std::string utils::timet2iso8601(time_t t)
{
    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&t));

    return std::string(buf);
}

std::string utils::userdir()
{
    char *home = getenv("HOME");
    if (!home)
        throw 0;

    return std::string(home);
}

std::string utils::appdir()
{
    std::ostringstream oss;
    oss << userdir();
    oss << "/.florb";

    return oss.str();
}

void utils::mkdir(const std::string& path)
{
    boost::filesystem::create_directory(path);
}

bool utils::exists(const std::string& path)
{
    return boost::filesystem::exists(path);
}

void utils::set_window_icon(Fl_Window *w)
{
    fl_open_display();
    Pixmap p, mask;
    XpmCreatePixmapFromData(fl_display, DefaultRootWindow(fl_display), const_cast<char**>(florb_xpm), &p, &mask, NULL);
    w->icon((char *)p);
}

// Not needed right now, might come in handy some time.
#if 0
bool utils::on_segment(point2d<double> pi, point2d<double> pj, point2d<double> pk) 
{
    return (pi.x() <= pk.x() || pj.x() <= pk.x()) && (pk.x() <= pi.x() || pk.x() <= pj.x()) &&
           (pi.y() <= pk.y() || pj.y() <= pk.y()) && (pk.y() <= pi.y() || pk.y() <= pj.y());
}

char utils::orientation(point2d<double> pi, point2d<double> pj, point2d<double> pk) 
{
    double a = (pk.x() - pi.x()) * (pj.y() - pi.y());
    double b = (pj.x() - pi.x()) * (pk.y() - pi.y());
    return a < b ? -1 : a > b ? 1 : 0;
}

bool utils::intersect(point2d<double> p1, point2d<double> p2, point2d<double> p3, point2d<double> p4) 
{
    char d1 = orientation(p3, p4, p1);
    char d2 = orientation(p3, p4, p2);
    char d3 = orientation(p1, p2, p3);
    char d4 = orientation(p1, p2, p4);
    return (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
            ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) ||
        (d1 == 0 && on_segment(p3, p4, p1)) ||
        (d2 == 0 && on_segment(p3, p4, p2)) ||
        (d3 == 0 && on_segment(p1, p2, p3)) ||
        (d4 == 0 && on_segment(p1, p2, p4));
}

bool utils::is_inside(point2d<unsigned long> p1, point2d<unsigned long> p2, point2d<unsigned long> ptest)
{
    if (ptest.x() < p1.x())
        return false;
    if (ptest.y() < p1.y())
        return false;
    if (ptest.x() >= p2.x())
        return false;
    if (ptest.y() >= p2.y())
        return false;

    return true;
}
#endif
