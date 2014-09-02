#ifndef MARKERLAYER_HPP
#define MARKERLAYER_HPP

#include "point.hpp"
#include "layer.hpp"
#include "viewport.hpp"

class markerlayer : public florb::layer
{
    public:
        markerlayer();
        ~markerlayer();

        class event_notify;

        bool draw(const viewport &viewport, florb::canvas &os);

        size_t add(const florb::point2d<double> &pmerc);
        void add(const florb::point2d<double> &pmerc, size_t id);
        void remove(size_t id);
        void clear();
    private:

        void notify();

        struct marker_internal {
            florb::point2d<double> p;
            size_t id;
        };

        std::vector<marker_internal> m_markers;
};

class markerlayer::event_notify : public event_base
{
    public:
        event_notify() {};
        ~event_notify() {};
};

#endif // MARKERLAYER_HPP

