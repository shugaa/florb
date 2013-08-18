#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

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
        void intersect(const viewport& vp);

        unsigned long x() const { return m_x; };
        unsigned long y() const { return m_y; };
        unsigned int z() const { return m_z; };
        unsigned long w() const { return m_w; };
        unsigned long h() const { return m_h; };

        bool operator == (const viewport& vp) {
            return cmp(vp);
        }
        bool operator != (const viewport& vp) {
            return !cmp(vp);   
        }

    private:
        bool cmp(const viewport& vp); 

        unsigned long m_x;
        unsigned long m_y;
        unsigned int m_z;
        unsigned long m_w;
        unsigned long m_h;
        unsigned long m_dim;
        unsigned int m_zmax;
        unsigned int m_zmin;
};

#endif // VIEWPORT_HPP

