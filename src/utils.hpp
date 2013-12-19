#ifndef UTILS_HPP
#define UTILS_HPP

#include <point.hpp>

class utils
{
    public:
        static unsigned long dim(unsigned int z);
        static int gps2merc(const point2d<double> &gps, point2d<double> &merc);
        static point2d<unsigned long> merc2px(unsigned int z, const point2d<double> &merc);
        static int gps2px(unsigned int z, const point2d<double> &gps, point2d<unsigned long> &px);
        static int px2gps(unsigned int z, const point2d<unsigned int> &px, point2d<double> &gps);
        static point2d<double> px2merc(unsigned int z, point2d<unsigned long> px);
        static double dist(point2d<double> p1, point2d<double> p2);
        static double dist_merc(point2d<double> p1, point2d<double> p2);
        static point2d<double> merc2gps(const point2d<double>& gps);

        static bool on_segment(point2d<double> pi, point2d<double> pj, point2d<double> pk);
        static char orientation(point2d<double> pi, point2d<double> pj, point2d<double> pk);
        static bool intersect(point2d<double> p1, point2d<double> p2, point2d<double> p3, point2d<double> p4);

        static bool is_inside(point2d<unsigned long> p1, point2d<unsigned long> p2, point2d<unsigned long> ptest);

    private:
};

#endif // UTILS_HPP
