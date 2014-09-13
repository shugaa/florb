#ifndef UNIT_HPP
#define UNIT_HPP

#include <string>

namespace florb
{
    class unit
    {
        public:

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

            static double convert(florb::unit::length src, florb::unit::length dst, double val);
            static std::string sistr(length spec);

        private:

            struct conv
            {
                int dst;
                double fac;
            };

            struct si
            {
                int unit;
                const char* si;
            };

            static double gen_convert(int src, int dst, double val, const florb::unit::conv *sc, size_t len);
            static std::string gen_sistr(int spec, const florb::unit::si *ssi, size_t len);

            static const florb::unit::conv m_conv_length[];
            static const florb::unit::si   m_si_length[];
    };
};

#endif // UNIT_HPP

