#include <cmath>
#include <sstream>
#include "gfx.hpp"
#include "utils.hpp"
#include "settings.hpp"
#include "point.hpp"
#include "gpsdlayer.hpp"

gpsdlayer::gpsdlayer() :
    layer(),
    m_pos(0.0, 0.0),
    m_track(0.0),
    m_mode(gpsdclient::FIX_NONE),
    m_valid(false),
    m_connected(false),
    m_gpsdclient(NULL)
{
    name(std::string("GPSD"));
    register_event_handler<gpsdlayer, gpsdclient::event_gpsd>(this, &gpsdlayer::handle_evt_gpsd);
};

gpsdlayer::~gpsdlayer()
{
    if (m_gpsdclient != NULL)
        delete m_gpsdclient;
};

const florb::point2d<double> gpsdlayer::pos()
{
    m_mutex.lock();
    florb::point2d<double> ptmp(m_pos);
    m_mutex.unlock();

    return ptmp;
}

void gpsdlayer::pos(florb::point2d<double> p) 
{
    m_mutex.lock();
    m_pos = p;
    m_mutex.unlock();
}

double gpsdlayer::track()
{
    m_mutex.lock();
    double ret = m_track;
    m_mutex.unlock();

    return ret;
}

void gpsdlayer::track(double t) 
{
    m_mutex.lock();
    m_track = t;
    m_mutex.unlock();
}

double gpsdlayer::mode()
{
    m_mutex.lock();
    int ret = m_mode;
    m_mutex.unlock();

    return ret;
}

void gpsdlayer::mode(int m) 
{
    m_mutex.lock();
    m_mode = m;
    m_mutex.unlock();
}

bool gpsdlayer::valid(void)
{
    m_mutex.lock();
    bool ret = m_valid;
    m_mutex.unlock();

    return ret;
}

void gpsdlayer::valid(bool v)
{
    m_mutex.lock();
    m_valid = v;
    m_mutex.unlock();
}

bool gpsdlayer::connected(void)
{
    m_mutex.lock();
    bool ret = m_connected;
    m_mutex.unlock();

    return ret;
}

void gpsdlayer::connected(bool c)
{
    m_mutex.lock();
    m_connected = c;
    m_mutex.unlock();
}

void gpsdlayer::connect(const std::string& host, const std::string& port)
{
    // Disconnect first if necessary
    disconnect();

    try {
    m_gpsdclient = new gpsdclient(host, port);
    } catch (std::runtime_error& e) {
        throw e;
    } 

    m_gpsdclient->add_event_listener(this);
}

void gpsdlayer::disconnect()
{
    if (m_gpsdclient != NULL)
    {
        delete m_gpsdclient;
        m_gpsdclient = NULL;
    }

    pos(florb::point2d<double>(0.0,0.0));
    track(0.0);
    mode(gpsdclient::FIX_NONE);
    valid(false);
    connected(false);

    fire_event_status();
}

void gpsdlayer::cb_fire_event_motion(void* userdata)
{
    // Are we pointing to a valid layer instance?
    layer *l = static_cast<layer*>(userdata);
    if (!is_instance(l))
        return;

    // Is the layer instance we're pointing to a gpsdlayer?
    gpsdlayer *gpsdl = dynamic_cast<gpsdlayer*>(l);
    if (!gpsdl)
        return;
    
    gpsdl->fire_event_motion();
}

void gpsdlayer::cb_fire_event_status(void* userdata)
{
    // Are we pointing to a valid layer instance?
    layer *l = static_cast<layer*>(userdata);
    if (!is_instance(l))
        return;

    // Is the layer instance we're pointing to a gpsdlayer?
    gpsdlayer *gpsdl = dynamic_cast<gpsdlayer*>(l);
    if (!gpsdl)
        return;
    
    gpsdl->fire_event_status();
}

void gpsdlayer::fire_event_motion()
{
    event_motion e(connected(), mode(), pos(), track());
    fire(&e);
}

void gpsdlayer::fire_event_status()
{
    event_status e(connected(), mode());
    fire(&e);
}

bool gpsdlayer::handle_evt_gpsd(const gpsdclient::event_gpsd *e)
{
    bool motion = false;

    // Found first fix or better quality fix
    if (e->mode() > mode()) 
        motion = true;
    // Motion >= 2m compared to the previous update
    else if (
        (valid()) &&
        ((e->mode() != gpsdclient::FIX_NONE)) && 
        (florb::utils::dist(pos(), e->pos()) >= 0.002))
        motion = true;
    
    connected(e->connected());
    mode(e->mode());
    track(e->track());

    if (motion) 
    {
        valid(true);
        pos(e->pos());
        Fl::awake(cb_fire_event_motion, this);
    }
    else
    {
        Fl::awake(cb_fire_event_status, this);
    }

    return true;
};

bool gpsdlayer::draw(const viewport &viewport, florb::canvas &os)
{
    if (!m_gpsdclient)
        return true;

    if (!valid())
        return true;

    // TODO: Performance killer!!
    florb::cfg_ui cfgui = florb::settings::get_instance()["ui"].as<florb::cfg_ui>(); 
    florb::color color_cursor(cfgui.gpscursorcolor());

    double t = track();
    double d2r = (M_PI/180.0);
    double csize = 17.0;

    // Calculate cursor rotation
    florb::point2d<int> p1((int)((cos((90.0+t)*d2r)*csize)), (int)(sin((90.0+t)*d2r)*csize));
    florb::point2d<int> p2((int)((cos((222.0+t)*d2r)*csize)), (int)(sin((222.0+t)*d2r)*csize));
    florb::point2d<int> p3((int)((cos((310.0+t)*d2r)*csize)), (int)(sin((310.0+t)*d2r)*csize));

    // Calculate current pixel position on the map
    florb::point2d<unsigned long> pxpos(florb::utils::wsg842px(viewport.z(), pos()));
    pxpos[0] -= viewport.x();
    pxpos[1] -= viewport.y();

    // Draw cursor
    os.fgcolor(color_cursor);
    os.line(pxpos.x()-p1.x(), pxpos.y()-p1.y(), pxpos.x()-p2.x(), pxpos.y()-p2.y(), 2);
    os.line(pxpos.x()-p2.x(), pxpos.y()-p2.y(), pxpos.x(), pxpos.y(), 2);
    os.line(pxpos.x(), pxpos.y(), pxpos.x()-p3.x(), pxpos.y()-p3.y(), 2);
    os.line(pxpos.x()-p3.x(), pxpos.y()-p3.y(), pxpos.x()-p1.x(), pxpos.y()-p1.y(), 2);

    return true;
};

