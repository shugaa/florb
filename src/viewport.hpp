#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

namespace florb
{
    class viewport
    {
        public:
            viewport();
            viewport(unsigned long w, unsigned long h);
            viewport(unsigned long x, unsigned long y, unsigned int z, unsigned long w, unsigned long h);
            ~viewport();

            void move(long dx, long dy);
            void x(unsigned long x);
            void y(unsigned long y);
            void w(unsigned long w);
            void h(unsigned long h);
            void z(unsigned int z, unsigned long x, unsigned long y);
            void intersect(const florb::viewport& vp);

            unsigned long x() const { return m_x; };
            unsigned long y() const { return m_y; };
            unsigned int z() const { return m_z; };
            unsigned long w() const { return m_w; };
            unsigned long h() const { return m_h; };

            bool operator == (const florb::viewport& vp) {
                return cmp(vp);
            }
            bool operator != (const florb::viewport& vp) {
                return !cmp(vp);   
            }
            bool operator < (const florb::viewport& vp) {
                if (m_w < vp.m_w) return true;
                if (m_h < vp.m_h) return true;
                return false;
            }

            // unsigned long range for map dimensions: at least 0 to
            // 4294967295. So the maximum zoomlevel can be
            // floor(log2(4294967295/256)) = 23. Zoomlevel 0 just results
            // in a 256x256 pixels size map. See florb::utils::dim() for
            // details.
            static const int ZMIN = 0;
            static const int ZMAX = 23;

        private:
            bool cmp(const florb::viewport& vp); 

            unsigned long m_x;
            unsigned long m_y;
            unsigned int  m_z;
            unsigned long m_w;
            unsigned long m_h;
            unsigned long m_dim;
    };
};

#endif // VIEWPORT_HPP

