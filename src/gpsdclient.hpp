#ifndef GPSDCLIENT_HPP
#define GPSDCLIENT_HPP

#include <string>
#include <set>
#include <boost/thread.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <gps.h>

// Forward declare observer class
class gpsdclient_observer;

class gpsdclient
{
    public:
        gpsdclient(const std::string host, const std::string port);
        ~gpsdclient();
        void addobserver(gpsdclient_observer &o);

        int mode(void);
        double latitude(void);
        double longitude(void);

        enum {
            FIX_NONE, 
            FIX_2D,
            FIX_3D,
        };
        enum {
            UPDATE_POS,
            UPDATE_MODE,
        };

    private:
        void worker(void);
        void notify_observers(void);

        bool handle_latlon(void);
        bool handle_mode(void);

        struct gps_data_t m_gpsdata;
        std::set<gpsdclient_observer*> m_observers;
        boost::interprocess::interprocess_mutex m_mutex; 

        std::string m_host;
        std::string m_port;
        boost::thread *m_thread;
        bool m_exit;

        int    m_mode;
        double m_latitude;
        double m_longitude;
};

class gpsdclient_observer
{
    public:
        virtual void gpsdclient_notify(void) = 0;
};

#endif // GPSDCLIENT_HPP
