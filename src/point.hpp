#ifndef POINT_HPP
#define POINT_HPP

#include <stdexcept>

template <class T>
class point2d
{
    public:
        point() :
            m_x(0), m_y(0) {;};
        point(T x, T y) :
            m_x(x), m_y(y) {;};
        point(const point& p) : 
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
                    throw std::out_of_range(idx);
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
        }

    private:
        T m_x;
        T m_y;
};

#endif // POINT_HPP

