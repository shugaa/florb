#include <cmath>
#include "utils.hpp"

int utils::gps2merc(const point2d<double> &gps, point2d<double> &merc)
{
    if ((gps.x() > 180.0) || (gps.x() < -180.0))
        return 1;
    if ((gps.y() > 90.0) || (gps.y() < -90.0))
        return 1;

    // Transform GPS to mercator
    merc.x(180.0 + gps.x());
    merc.y(180.0 + (180.0/M_PI * log(tan(M_PI/4.0+gps.y()*(M_PI/180.0)/2.0))));

    return 0;
}

point2d<unsigned long> utils::merc2px(unsigned int z, const point2d<double> &merc)
{
    unsigned long dimxy = dim(z);

    return point2d<unsigned long>(
        (unsigned long)(((double)dimxy/360.0) * merc.x()),
        (unsigned long)(dimxy-(((double)dimxy/360.0) * merc.y())));
}

int utils::gps2px(unsigned int z, const point2d<double> &gps, point2d<unsigned long> &px)
{
    // Transform GPS to mercator
    point2d<double> merc;
    int rc = gps2merc(gps, merc);
    if (rc != 0)
        return 1;

    // Tranxform mercator to pixel position
    px = merc2px(z, merc);

    return 0;
}

int utils::px2gps(unsigned int z, const point2d<unsigned int> &px, point2d<double> &gps)
{
    // Get map dimensions
    unsigned long dimxy = dim(z);

    // Make sure the coordinate is on the map
    if ((px.x() > dimxy) || (px.y() > dimxy))
        return 1;

    // Convert pixel position to mercator coordinate
    double mlon = (360.0/((double)dimxy/(double)px.x()));
    double mlat = (360.0/((double)dimxy/(double)(dimxy-px.y())));

    // Convert mercator to GPS coordinate
    gps.x(mlon - 180.0);
    gps.y(180.0/M_PI * (2.0 * atan(exp((mlat-180.0)*M_PI/180.0)) - M_PI/2.0));

    return 0;
}

point2d<double> utils::px2merc(unsigned int z, point2d<unsigned long> px)
{
    unsigned long dimxy = dim(z);

    // Make sure the coordinate is on the map
    if ((px.x() > dimxy) || (px.y() > dimxy))
        throw 1;

    double mlon = (360.0/((double)dimxy/(double)px.x()));
    double mlat = (360.0/((double)dimxy/(double)(dimxy-px.y())));
    return point2d<double>(mlon, mlat);
}

unsigned long utils::dim(unsigned int z)
{
    return pow(2.0, z) * 256;
}


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

/** Do line segments (x1, y1)--(x2, y2) and (x3, y3)--(x4, y4) intersect? */
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
