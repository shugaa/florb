#include <boost/bind.hpp>

#include "gpsdclient.hpp"

gpsdclient::gpsdclient(const std::string host, const std::string port) : 
    m_host(host),
    m_port(port),
    m_thread(NULL),
    m_exit(false),
    m_mode(FIX_NONE),
    m_latitude(0.0),
    m_longitude(0.0)
{
    m_thread = new boost::thread(boost::bind(&gpsdclient::worker, this));
}

gpsdclient::~gpsdclient()
{
    if (m_thread) 
    {
        m_exit = true;
        m_thread->join();
        delete m_thread;
    }
}

void gpsdclient::addobserver(gpsdclient_observer &o)
{
    m_observers.insert(&o);
}

double gpsdclient::latitude(void)
{
    double ret;

    m_mutex.lock();
    ret = m_latitude;
    m_mutex.unlock();

    return ret;
}

double gpsdclient::longitude(void)
{
    double ret;

    m_mutex.lock();
    ret = m_longitude;
    m_mutex.unlock();

    return ret;    
}

int gpsdclient::mode(void)
{
    int ret;

    m_mutex.lock();
    ret = m_mode;
    m_mutex.unlock();

    return ret;  
}

void gpsdclient::notify_observers(void)
{
    std::set<gpsdclient_observer*>::iterator it;
    for (it = m_observers.begin(); it != m_observers.end(); it++)
        (*it)->gpsdclient_notify();
}

void gpsdclient::worker(void)
{

    int rc;
    
    for (;;)
    {
        rc = gps_open(m_host.c_str(), m_port.c_str(), &m_gpsdata);
        if (rc < 0)
            break;
        rc = gps_stream(&m_gpsdata, WATCH_ENABLE | WATCH_JSON, NULL);
        if (rc < 0)
            break;

        break;
    }

    if (rc < 0)
        return;

    for (;;)
    {
        bool notify = false;

        if (m_exit == true)
            break;

        if (!gps_waiting(&m_gpsdata, 5000))
            continue;

        if (gps_read(&m_gpsdata) == -1)
            continue;

        if (m_gpsdata.set & LATLON_SET) 
        {
            notify = handle_latlon();
        }
        else if (m_gpsdata.set & MODE_SET)
        {
            handle_mode();
        }

        // Notify observers
        if (notify)
        {
            notify_observers();
        }
    }

    gps_stream(&m_gpsdata, WATCH_DISABLE, NULL);
    gps_close (&m_gpsdata);
}

bool gpsdclient::handle_latlon(void)
{
    bool ret = false;

    m_mutex.lock();

    if (m_latitude != m_gpsdata.fix.latitude)
        ret = true;
    if (m_longitude != m_gpsdata.fix.longitude)
        ret = true;

    m_latitude = m_gpsdata.fix.latitude;
    m_longitude = m_gpsdata.fix.longitude;

    m_mutex.unlock();

    return ret;
}

bool gpsdclient::handle_mode(void)
{
    bool ret = false;
    int  mode = FIX_NONE;

    m_mutex.lock();

    if (m_gpsdata.fix.mode ==  MODE_2D)
        mode = FIX_2D;
    else if (m_gpsdata.fix.mode == MODE_3D)
        mode = FIX_3D;

    if (mode != m_mode)
    {
        m_mode = mode;
        ret = true;
    }

    m_mutex.unlock();

    return ret;
}


