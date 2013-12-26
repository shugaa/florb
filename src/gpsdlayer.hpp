#ifndef GPSDLAYER_HPP
#define GPSDLAYER_HPP

#include <boost/interprocess/sync/interprocess_mutex.hpp>

#include "point.hpp"
#include "layer.hpp"
#include "viewport.hpp"
#include "gpsdclient.hpp"

class gpsdlayer : public layer
{
    public:
        gpsdlayer();
        ~gpsdlayer();

        void draw(const viewport &viewport,canvas &os);

        const point2d<double> pos();
        double track(); 
        double mode();
        bool valid();

        class event_motion;
    private:
        void pos(point2d<double> p);
        void track(double t); 
        void mode(int m);
        void valid(bool v);

        static void cb_fire_event_motion(void* userdata);
        void fire_event_motion();

        point2d<double> m_pos;
        double m_track;
        int m_mode;
        bool m_valid;

        bool handle_evt_gpsupdate(const gpsdclient::event_update *e);
        gpsdclient *m_gpsdclient;

        boost::interprocess::interprocess_mutex m_mutex;
};

class gpsdlayer::event_motion : public event_base
{
    public:
        event_motion(int mode, const point2d<double>& pos, double track) :
            m_mode(mode),
            m_pos(pos),
            m_track(track) {};
        ~event_motion() {};

        int mode() const { return m_mode; };
        const point2d<double>& pos() const { return m_pos; };
        double track() const { return m_track; };

    private:
        int m_mode;
        point2d<double> m_pos;
        double m_track;
};

#endif // GPSDLAYER_HPP

