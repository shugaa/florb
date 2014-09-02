#ifndef AREASELECTLAYER_HPP
#define AREASELECTLAYER_HPP

#include "layer.hpp"
#include "viewport.hpp"

class areaselectlayer : public florb::layer
{
    public:
        areaselectlayer();
        ~areaselectlayer();

        class event_done;
        class event_notify;

        bool handle_evt_mouse(const layer::event_mouse* evt);

        void clear();
        bool draw(const viewport &viewport, florb::canvas &os);
    private:

        bool press(const layer::event_mouse* evt);
        bool release(const layer::event_mouse* evt);
        bool drag(const layer::event_mouse* evt);

        void done();
        void notify();

        std::string m_caption;
        florb::point2d<unsigned long> m_p1, m_p2;
        viewport m_vp;
};

class areaselectlayer::event_done : public event_base
{
    public:
        event_done(const viewport& vp) : m_vp(vp) {};
        ~event_done() {};
        
        void vp(const viewport& vp) {m_vp = vp;};
        const viewport& vp() const { return m_vp; };
    private:
        viewport m_vp;
};

class areaselectlayer::event_notify : public event_base
{
    public:
        event_notify() {};
        ~event_notify() {};
};


#endif // AREASELECTLAYER_HPP

