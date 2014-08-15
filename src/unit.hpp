#ifndef UNIT_HPP
#define UNIT_HPP

#include <string>

class unit
{
    public:

        static double convert(int src, int dst, double val);
        static std::string sistr(int spec);

        enum system
        {
            METRIC,
            IMPERIAL,
            NAUTICAL,

            /* Always last */
            ENUM_SYSTEM_END
        };

        enum length {
            M,
            KM,
            ENGLISH_MILE,
            US_MILE,
            SEA_MILE,
            YARD,
            INCH,
            FOOT,

            /* Always last */
            ENUM_LENGTH_END
        };

    private:

        struct conv
        {
            int src;
            int dst;
            double fac;
        };

        struct si
        {
            int unit;
            const char* si;
        };

        static const conv m_conv_length[];
        static const si   m_si_length[];
};

#endif // UNIT_HPP

