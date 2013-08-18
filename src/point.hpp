#ifndef POINT_HPP
#define POINT_HPP

template <class T>
class point
{
    public:
        point() :
            m_x(0), m_y(0) {;};
        point(T x, T y) :
            m_x(x), m_y(y) {;};
        point(const point &point) : 
            m_x(point.get_x()), m_y(point.get_y()) {;};

        T get_x() const { return m_x; };
        T get_y() const { return m_y; };

        void set_x(T x) { m_x = x; };
        void set_y(T y) { m_y = y; };

        void add(const T &other) {
            m_x += other.get_x();
            m_y += other.get_y();
        }

    private:
        T m_x;
        T m_y;
};

template <class T>
class orb_area
{
    public:
        orb_area(T w, T h) :
            m_w(w), m_h(h) {;};
        orb_area(const orb_area &area) : 
            m_w(area.get_w()), m_h(area.get_h()) {;};

        T get_w() const { return m_w; };
        T get_h() const { return m_h; };

        void set_w(T w) { m_w = w; };
        void set_h(T h) { m_h = h; };

    private:
        T m_w;
        T m_h;
};

#endif // POINT_HPP

