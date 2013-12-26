#ifndef GPSDCLIENT_HPP
#define GPSDCLIENT_HPP

#include <string>
#include <set>
#include <boost/thread.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <gps.h>
#include "event.hpp"
#include "point.hpp"

class gpsdclient : public event_generator
{
    public:
        gpsdclient(const std::string host, const std::string port);
        ~gpsdclient();

        bool connected(void);

        enum {
            FIX_NONE, 
            FIX_2D,
            FIX_3D,
        };

        class event_update;

    private:
        bool exit();
        void exit(bool e);

        void worker(void);
        bool handle_set(void);

        void fire_event_update(void);

        struct gps_data_t m_gpsdata;
        boost::interprocess::interprocess_mutex m_mutex; 

        std::string m_host;
        std::string m_port;
        boost::thread *m_thread;
        bool m_exit;
        bool m_connected;

        int    m_mode;
        double m_latitude;
        double m_longitude;
        double m_track;
};

class gpsdclient::event_update : public event_base
{
    public:
        event_update(int mode, const point2d<double>& pos, double track) :
            m_mode(mode),
            m_pos(pos),
            m_track(track) {};
        ~event_update() {};

        int mode() const { return m_mode; };
        const point2d<double>& pos() const { return m_pos; };
        double track() const { return m_track; };

    private:
        int m_mode;
        point2d<double> m_pos;
        double m_track;
};

#endif // GPSDCLIENT_HPP
