#ifndef UTILS_HPP
#define UTILS_HPP

#include <ctime>
#include <vector>
#include <libintl.h>
#include <FL/Fl_Window.H>
#include "point.hpp"

#define _(STRING) gettext(STRING)

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

        static bool clipline(point2d<double> &p1, point2d<double> &p2, point2d<double> r1, point2d<double> r2, bool &p1clip, bool &p2clip); 
        
        static double dist(const point2d<double> &p1, const point2d<double> &p2);
        static double dist_merc(const point2d<double> &p1, const point2d<double> &p2);

        static time_t iso8601_2timet(const std::string& iso);
        static std::string timet2iso8601(time_t t);

        static std::vector<std::string> str_split(const std::string& str, const std::string& delimiter);
        static std::size_t str_count(const std::string& str, const std::string& token);

        static std::string pathsep();
        static std::string userdir();
        static std::string appdir();
        static void mkdir(const std::string& path);
        static void rm(const std::string& path);
        static bool exists(const std::string& path);
        static std::string filestem(const std::string& path);
        static void touch(const std::string& path);

        static void set_window_icon(Fl_Window *w);
    private:
};

#endif // UTILS_HPP
