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
        gpsdlayer(const std::string& host, const std::string& port);
        ~gpsdlayer();

        bool draw(const viewport &viewport, florb::canvas &os);

        const point2d<double> pos();
        double track(); 
        double mode();
        bool valid();
        bool connected();

        class event_status;
        class event_motion;
    private:
        void pos(point2d<double> p);
        void track(double t); 
        void mode(int m);
        void valid(bool v);
        void connected(bool c);

        static void cb_fire_event_motion(void* userdata);
        static void cb_fire_event_status(void* userdata);
        void fire_event_motion();
        void fire_event_status();

        point2d<double> m_pos;
        double m_track;
        int m_mode;
        bool m_valid;
        bool m_connected;

        bool handle_evt_gpsd(const gpsdclient::event_gpsd *e);
        gpsdclient *m_gpsdclient;

        boost::interprocess::interprocess_mutex m_mutex;
};

class gpsdlayer::event_status : public event_base
{
    public:
        event_status(bool connected, int mode) :
            m_connected(connected),
            m_mode(mode) {};
        virtual ~event_status() {};

        bool connected() const { return m_connected; }
        int mode() const { return m_mode; };

    private:
        bool m_connected;
        int m_mode;
};

class gpsdlayer::event_motion : public event_status
{
    public:
        event_motion(bool connected, int mode, const point2d<double>& pos, double track) :
            event_status(connected, mode),
            m_pos(pos),
            m_track(track) {};
        ~event_motion() {};

        const point2d<double>& pos() const { return m_pos; };
        double track() const { return m_track; };

    private:
        point2d<double> m_pos;
        double m_track;
};

#endif // GPSDLAYER_HPP

