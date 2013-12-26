#include "gpsdclient.hpp"

gpsdclient::gpsdclient(const std::string host, const std::string port) : 
    m_host(host),
    m_port(port),
    m_thread(NULL),
    m_exit(false),
    m_connected(false),
    m_mode(FIX_NONE),
    m_latitude(0.0),
    m_longitude(0.0),
    m_track(0.0)
{
    m_thread = new boost::thread(boost::bind(&gpsdclient::worker, this));
}

gpsdclient::~gpsdclient()
{
    if (m_thread) 
    {
        exit(true);
        m_thread->join();
        delete m_thread;
    }
}

bool gpsdclient::exit()
{
    m_mutex.lock();
    bool ret = m_exit;
    m_mutex.unlock();

    return ret;
}
void gpsdclient::exit(bool e)
{
    m_mutex.lock();
    m_exit = e;
    m_mutex.unlock();
}

bool gpsdclient::connected(void)
{
    bool ret;

    m_mutex.lock();
    ret = m_connected;
    m_mutex.unlock();

    return ret;   
}

void gpsdclient::fire_event_update(void)
{
    event_update ue(
            m_mode,
            point2d<double>(m_longitude, m_latitude),
            m_track);

    fire(&ue);
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

    // Update connection status
    if (rc == 0) {
        m_mutex.lock();
        m_connected = true;
        m_mutex.unlock();
    }

    for (;;)
    {
        // Exit request
        if (exit())
            break;

        // Initialisation failure
        if (rc != 0)
            break;

        // Wait for data, 200ms timeout
        if (!gps_waiting(&m_gpsdata, 200000)) {
            // Error
            if (errno != 0)
                break;

            // Normal timeout
            continue;
        }

        // Read data
        if (gps_read(&m_gpsdata) == -1) {
            // Error
            break;
        }

        handle_set();
    }

    // Try to shut down
    gps_stream(&m_gpsdata, WATCH_DISABLE, NULL);
    gps_close(&m_gpsdata);

    // Update connection status
    m_mutex.lock();
    m_connected = false;
    m_mutex.unlock();
}

bool gpsdclient::handle_set()
{
    bool ret = false;

    for (;;) {
        // Handle latitude / longitude set
        if (m_gpsdata.set & LATLON_SET)
        {
            if (m_gpsdata.set & MODE_SET)
            {
                int mode = FIX_NONE;
                if (m_gpsdata.fix.mode == MODE_2D) 
                {
                    m_mutex.lock();
                    mode = FIX_2D;
                    m_mutex.unlock();
                } else if (m_gpsdata.fix.mode == MODE_3D) 
                {
                    m_mutex.lock();
                    mode = FIX_3D;
                    m_mutex.unlock();                
                }

                if (m_mode != mode)
                    ret = true;

                m_mutex.lock();
                m_mode = mode;
                m_mutex.unlock();

                // Handle latitude / longitude
                if ((m_gpsdata.fix.latitude != m_latitude) ||
                    (m_gpsdata.fix.longitude != m_longitude)) 
                {
                    m_mutex.lock();
                    m_latitude = m_gpsdata.fix.latitude;
                    m_longitude = m_gpsdata.fix.longitude;
                    m_mutex.unlock();
                    ret = true;
                }
            }
        }

        // Handle track / course over ground info
        if (m_gpsdata.set & TRACK_SET)
        {
            if (m_track != m_gpsdata.fix.track) {
                m_mutex.lock();
                m_track = m_gpsdata.fix.track;
                m_mutex.unlock();
                ret = true;
            }
        }

        if (ret)
            fire_event_update();

        break;
    }

    return ret;
}

