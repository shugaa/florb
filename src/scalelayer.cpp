#include <cmath>
#include <sstream>
#include "gfx.hpp"
#include "utils.hpp"
#include "point.hpp"
#include "unit.hpp"
#include "settings.hpp"
#include "scalelayer.hpp"

florb::scalelayer::scalelayer()
{
    name(std::string("Scale"));
};

florb::scalelayer::~scalelayer()
{
};

bool florb::scalelayer::draw(const viewport &viewport, florb::canvas &os)
{
    // Calculate coordinate in the center of the viewport
    florb::point2d<unsigned long> ppx(viewport.x() + (viewport.w()/2), viewport.y() + (viewport.h()/2));
    florb::point2d<double> pwsg84(florb::utils::px2wsg84(viewport.z(), ppx));

    // Calculate meters per pixel for this latitude and zoom level
    double mpp = florb::utils::meters_per_pixel(viewport.z(), pwsg84.y());

    // Length of the scale in meters
    double slm = 0.0;

    // Length of the scale in pixels
    unsigned int slp = 0;

    // Maintain a roughly 100 pixels long scale
    while (slp < 100)
    {
        for (;;)
        {
            // 10 m increase
            double delta = 10.0;

            // <= 100 m
            if ((slm + delta) <= 100.0)
            {
                slm += delta;
                break;
            }

            // 50 m increase
            delta = 50.0;

            // <= 1.0 km
            if ((slm + delta) <= 1000.0)
            {
                slm += delta;
                break;
            }

            // 100 m increase
            delta = 100.0;

            // <= 1.5 km
            if ((slm + delta) <= 2000.0)
            {
                slm += delta;
                break;
            }

            // 500 m increase
            delta = 1000.0;

            // <= 10.0 km
            if ((slm + delta) <= 10000.0)
            {
                slm += delta;
                break;
            }

            // 1000 m increase
            delta = 5000.0;

            // <= 100.0 km
            if ((slm + delta) <= 100000.0)
            {
                slm += delta;
                break;
            }

            // 50000 m increase
            delta = 50000.0;
            slm += delta;

            break;
        }

        slp = (unsigned int)(slm / mpp);
    }

#if 0
    std::cout << " 1 pixel is " << mpp << " meters" << std::endl;
    std::cout << slp << " pixels are " << slm << " meters" << std::endl;
#endif

    // Set color and font size
    os.fgcolor(florb::color(0,0,0));
    os.fontsize(12);

    // Draw the scale itself
    os.rect(
        20,
        viewport.h()-20, 
        slp, 
        5);

    // Generate the info text
    unit::length usrc = unit::length::M;
    if (slm >= 1000.0)
    {
        slm /= 1000.0;
        usrc = unit::length::KM;
    }

    florb::cfg_units cfgunits = florb::settings::get_instance()["units"].as<florb::cfg_units>();

    unit::length udst = unit::length::KM;
    switch (cfgunits.system_length())
    {
        case (florb::cfg_units::system::IMPERIAL):
            udst = (usrc == unit::length::M) ? unit::length::FOOT : unit::length::ENGLISH_MILE;
            break;
        case (florb::cfg_units::system::NAUTICAL):
            udst = (usrc == unit::length::M) ? unit::length::FOOT : unit::length::SEA_MILE;
            break;
        default:
            udst = (usrc == unit::length::M) ? unit::length::M : unit::length::KM;
            break;
    }

    std::ostringstream oss;
    oss.precision(1);
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss << unit::convert(usrc, udst, slm) << " " << unit::sistr(udst);

    // Draw the info text
    os.text(oss.str(), 20, viewport.h()-34);

    return true;
};

