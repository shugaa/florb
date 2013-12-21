#ifndef GPSDCLIENT_HPP
#define GPSDCLIENT_HPP

#include <string>
#include <set>
#include <boost/thread.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <gps.h>
#include "event.hpp"

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

    private:
        void worker(void);
        bool handle_set(void);

        void event_update(void);

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

class gpsdclient_update_event : public event_base
{
    public:
        int mode;
        double latitude;
        double longitude;
        double track;
};

#endif // GPSDCLIENT_HPP
