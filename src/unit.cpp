#include <stdexcept>
#include "utils.hpp"
#include "unit.hpp"

const florb::unit::si florb::unit::m_si_length[] =
{
    {M,             _("m")},
    {KM,            _("km")},
    {ENGLISH_MILE,  _("mi")},
    {US_MILE,       _("mi")},
    {SEA_MILE,      _("sm")},
    {YARD,          _("yd")},
    {INCH,          _("in")},
    {FOOT,          _("ft")},
};

const florb::unit::conv florb::unit::m_conv_length[] =
{
    {M,                 1.0},
    {KM,                1000.0},
    {ENGLISH_MILE,      1609.344},
    {SEA_MILE,          1852.0},
    {US_MILE,           1609.347219},
    {YARD,              0.9144},
    {INCH,              0.0254},
    {FOOT,              0.3048},
};

double florb::unit::gen_convert(int src, int dst, double val, const florb::unit::conv *sc, size_t len)
{
    // Convert src to base unit
    for (size_t i=0;i<len;i++)
    {
        if (m_conv_length[i].dst == src)
        {
            val *= sc[i].fac;
            break;
        }
    }

    // Convert to destination unit
    bool found = false;
    for (size_t i=0;i<len;i++)
    {
        if (m_conv_length[i].dst == dst)
        {
            val /= sc[i].fac;
            found = true;
            break;
        }
    }

    if (!found)
        throw std::runtime_error(_("Invalid unit conversion"));

    return val;
}

std::string florb::unit::gen_sistr(int spec, const florb::unit::si *ssi, size_t len)
{
    std::string ret;
    bool found = false;
    for (size_t i=0;i<len;i++)
    {
        if (ssi[i].unit == spec)
        {
            ret = ssi[i].si;
            found = true;
            break;
        }
    }

    if (!found)
        throw std::runtime_error(_("Invalid unit conversion"));

    return _(ret.c_str());
}

double florb::unit::convert(florb::unit::length src, florb::unit::length dst, double val)
{
    return gen_convert(
            static_cast<int>(src), 
            static_cast<int>(dst), 
            val, 
            m_conv_length, 
            sizeof(m_conv_length)/sizeof(m_conv_length[0]));
}

std::string florb::unit::sistr(florb::unit::length spec)
{
    return gen_sistr(
            static_cast<int>(spec), 
            m_si_length,
            sizeof(m_si_length)/sizeof(m_si_length[0]));
}

