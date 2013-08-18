#ifndef UTILS_HPP
#define UTILS_HPP

#include <point.hpp>

class utils
{
    public:
        static unsigned long dim(unsigned int z);
        static int gps2merc(const point<double> &gps, point<double> &merc);
        static int merc2px(unsigned int z,const point<double> &merc, point<unsigned int> &px);
        static int gps2px(unsigned int z, const point<double> &gps, point<unsigned int> &px);
        static int px2gps(unsigned int z, const point<unsigned int> &px, point<double> &gps);
    
    private:
};

#endif // UTILS_HPP
