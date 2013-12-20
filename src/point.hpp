#ifndef POINT_HPP
#define POINT_HPP

#include <stdexcept>

template <class T>
class point2d
{
    public:
        point2d() :
            m_x(0), m_y(0) {;};
        point2d(T x, T y) :
            m_x(x), m_y(y) {;};
        point2d(const point2d& p) : 
            m_x(p.m_x), m_y(p.m_y) {;};

        T& operator[] (int idx) 
        {
            switch (idx)
            {
                case 0:
                    return m_x;
                case 1:
                    return m_y;
                default:
                    throw std::out_of_range(0);
            }
        };

        point2d<T> operator+(const point2d<T>& p)
        {
            return point2d<T>(m_x + p.m_x, m_y + p.m_y);
        };

        point2d<T>& operator= (const point2d<T>& p)
        {
            m_x = p.m_x;
            m_y = p.m_y;
            return *this;
        };

        bool operator==(const point2d<T>& p) const
        {
            return ((m_x == p.m_x) && (m_y == p.m_y));
        };

        T x() const { return m_x; };
        T y() const { return m_y; };
        void x(const T& sx) { m_x = sx; };
        void y(const T& sy) { m_y = sy; };

    private:
        T m_x;
        T m_y;
};

#endif // POINT_HPP

