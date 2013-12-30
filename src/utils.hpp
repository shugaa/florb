#ifndef UTILS_HPP
#define UTILS_HPP

#include <ctime>
#include <FL/Fl_Window.H>
#include "point.hpp"

class utils
{
    public:
        static unsigned long dim(unsigned int z);

        static point2d<double> wsg842merc(const point2d<double> &wsg84);
        static point2d<unsigned long> merc2px(unsigned int z, const point2d<double> &merc);
        static point2d<unsigned long> wsg842px(unsigned int z, const point2d<double> &wsg84);
        static point2d<double> px2wsg84(unsigned int z, const point2d<unsigned long> &px);
        static point2d<double> merc2wsg84(const point2d<double>& wsg84);
        static point2d<double> px2merc(unsigned int z, const point2d<unsigned long> &px);
        
        static double dist(const point2d<double> &p1, const point2d<double> &p2);
        static double dist_merc(const point2d<double> &p1, const point2d<double> &p2);

        static time_t iso8601_2timet(const std::string& iso);
        static std::string timet2iso8601(time_t t);

        static std::string userdir();
        static std::string appdir();
        static void mkdir(const std::string& path);
        static bool exists(const std::string& path);

        static void set_window_icon(Fl_Window *w);

#if 0
        static bool on_segment(point2d<double> pi, point2d<double> pj, point2d<double> pk);
        static char orientation(point2d<double> pi, point2d<double> pj, point2d<double> pk);
        static bool intersect(point2d<double> p1, point2d<double> p2, point2d<double> p3, point2d<double> p4);
        static bool is_inside(point2d<unsigned long> p1, point2d<unsigned long> p2, point2d<unsigned long> ptest);
#endif

    private:
};

#endif // UTILS_HPP
