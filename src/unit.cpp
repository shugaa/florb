#include <stdexcept>
#include "utils.hpp"
#include "unit.hpp"

const unit::si unit::m_si_length[] =
{
    {M,             "m"},
    {KM,            "km"},
    {ENGLISH_MILE,  "mi"},
    {US_MILE,       "mi"},
    {SEA_MILE,      "sm"},
    {YARD,          "yd"},
    {INCH,          "in"},
    {FOOT,          "ft"}
};

const unit::conv unit::m_conv_length[] =
{
    {M, KM,                1000.0},
    {M, ENGLISH_MILE,      1609.344},
    {M, SEA_MILE,          1852.0},
    {M, US_MILE,           1609.347219},
    {M, YARD,              0.9144},
    {M, INCH,              0.0254},
    {M, FOOT,              0.3048}
};

double unit::convert(int src, int dst, double val)
{
    // Convert src to meters if it isn't already
    bool found = false;
    if (src != M)
    {
        for (size_t i=0;i<(sizeof(m_conv_length)/sizeof(m_conv_length[0]));i++)
        {
            if (m_conv_length[i].dst == src)
            {
                val *= m_conv_length[i].fac;
                found = true;
                break;
            }
        }

        if (!found)
            throw std::runtime_error(_("Invalid unit conversion"));
    }


    // Convert to destination unit
    found = false;
    for (size_t i=0;i<(sizeof(m_conv_length)/sizeof(m_conv_length[0]));i++)
    {
        if (m_conv_length[i].dst == dst)
        {
            val /= m_conv_length[i].fac;
            found = true;
            break;
        }
    }

    if (!found)
        throw std::runtime_error(_("Invalid unit conversion"));

    return val;
}

std::string unit::sistr(int spec)
{
    std::string ret;
    bool found = false;
    for (size_t i=0;i<(sizeof(m_si_length)/sizeof(m_si_length[0]));i++)
    {
        if (m_si_length[i].unit == spec)
        {
            ret = m_si_length[i].si;
            found = true;
            break;
        }
    }

    if (!found)
        throw std::runtime_error(_("Invalid unit conversion"));

    return ret;
}

