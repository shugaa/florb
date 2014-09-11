#ifndef GPSDCLIENT_HPP
#define GPSDCLIENT_HPP

#include <string>
#include <boost/thread.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <gps.h>
#include "event.hpp"
#include "point.hpp"

namespace florb
{
    class gpsdclient : public event_generator
    {
        public:
            gpsdclient(const std::string host, const std::string port);
            ~gpsdclient();

            bool connected(void);
            int mode(void);
            florb::point2d<double> pos(void);
            double track(void);

            enum {
                FIX_NONE, 
                FIX_2D,
                FIX_3D,
            };

            class event_gpsd;

        private:
            void connected(bool c);
            void mode(int m);
            void pos(const florb::point2d<double>& p);
            void pos(double lon, double lat);
            void track(double t);

            bool exit();
            void exit(bool e);
            void worker(void);
            bool handle_set(void);

            void fire_event_gpsd(void);

            struct gps_data_t m_gpsdata;
            boost::interprocess::interprocess_mutex m_mutex; 

            std::string m_host;
            std::string m_port;
            boost::thread *m_thread;

            bool m_exit;
            bool m_connected;
            int m_mode;
            florb::point2d<double> m_pos;
            double m_track;
    };

    class gpsdclient::event_gpsd : public event_base
    {
        public:
            event_gpsd(bool c, int mode, const florb::point2d<double>& pos, double track) :
                m_connected(c),
                m_mode(mode),
                m_pos(pos),
                m_track(track) {};
            ~event_gpsd() {};

            int mode() const { return m_mode; };
            const florb::point2d<double>& pos() const { return m_pos; };
            double track() const { return m_track; };
            bool connected() const { return m_connected; };

        private:
            bool m_connected;
            int m_mode;
            florb::point2d<double> m_pos;
            double m_track;
    };
};

#endif // GPSDCLIENT_HPP

