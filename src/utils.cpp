#include <cmath>
#include "utils.hpp"

int utils::gps2merc(const point<double> &gps, point<double> &merc)
{
    if ((gps.get_x() > 180.0) || (gps.get_x() < -180.0))
        return 1;
    if ((gps.get_y() > 90.0) || (gps.get_y() < -90.0))
        return 1;

    // Transform GPS to mercator
    merc.set_x(180.0 + gps.get_x());
    merc.set_y(180.0 + (180.0/M_PI * log(tan(M_PI/4.0+gps.get_y()*(M_PI/180.0)/2.0))));

    return 0;
}

int utils::merc2px(unsigned int z, const point<double> &merc, point<unsigned int> &px)
{
    unsigned long dimxy = dim(z);

    // Get the pixel position on the map for the reference lat/lon
    px.set_x((unsigned int)(((double)dimxy/360.0) * merc.get_x()));
    px.set_y((unsigned int)(dimxy-(((double)dimxy/360.0) * merc.get_y())));

    return 0;
}

int utils::gps2px(unsigned int z, const point<double> &gps, point<unsigned int> &px)
{
    // Transform GPS to mercator
    point<double> merc;
    int rc = gps2merc(gps, merc);
    if (rc != 0)
        return 1;

    // Tranxform mercator to pixel position
    merc2px(z, merc, px);

    return 0;
}

int utils::px2gps(unsigned int z, const point<unsigned int> &px, point<double> &gps)
{
    // Get map dimensions
    unsigned long dimxy = dim(z);

    // Make sure the coordinate is on the map
    if ((px.get_x() > dimxy) || (px.get_y() > dimxy))
        return 1;

    // Convert pixel position to mercator coordinate
    double mlon = (360.0/((double)dimxy/(double)px.get_x()));
    double mlat = (360.0/((double)dimxy/(double)(dimxy-px.get_y())));

    // Convert mercator to GPS coordinate
    gps.set_x(mlon - 180.0);
    gps.set_y(180.0/M_PI * (2.0 * atan(exp((mlat-180.0)*M_PI/180.0)) - M_PI/2.0));

    return 0;
}

point2d<double> utils::px2merc(unsigned int z, point2d<unsigned long> px)
{
    unsigned long dimxy = dim(z);
    double mlon = (360.0/((double)dimxy/(double)px.get_x()));
    double mlat = (360.0/((double)dimxy/(double)(dimxy-px.get_y())));
    return point2d(mlon, mlat);
}

unsigned long utils::dim(unsigned int z)
{
    return pow(2.0, z) * 256;
}

