#include <cmath>
#include <sstream>
#include "gfx.hpp"
#include "utils.hpp"
#include "settings.hpp"
#include "point.hpp"
#include "gpsdlayer.hpp"

gpsdlayer::gpsdlayer() :
    layer(),
    m_mode(gpsdclient::FIX_NONE),
    m_valid(false),
    m_gpsdclient(NULL)
{
    name(std::string("Unnamed GPSD layer"));

    m_gpsdclient = new gpsdclient("localhost", "2947");

    if (m_gpsdclient)
        m_gpsdclient->add_event_listener(this);

    register_event_handler<gpsdlayer, gpsdclient::event_update>(this, &gpsdlayer::handle_evt_gpsupdate);
};

gpsdlayer::~gpsdlayer()
{
    if (m_gpsdclient)
        delete m_gpsdclient;
};

//void gpsdlayer::connect(const std::sting& host, int port)
//{
//    disconnect();
//    m_gpsdclient = new gpsdclient(host, port);
//}

//void gpsdlayer::disconnect()
//{
//    if (m_gpsdclient)
//        delete m_gpsdclient;
//}

const point2d<double> gpsdlayer::pos()
{
    m_mutex.lock();
    point2d<double> ptmp(m_pos);
    m_mutex.unlock();

    return ptmp;
}
void gpsdlayer::pos(point2d<double> p) 
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

void gpsdlayer::fire_event_motion()
{
    event_motion e(mode(), pos(), track());
    fire(&e);
}

bool gpsdlayer::handle_evt_gpsupdate(const gpsdclient::event_update *e)
{
    bool motion = false;

    // Found fix
    if ((e->mode() != mode()) && (mode() == gpsdclient::FIX_NONE)) 
        motion = true;
    // Motion > 1m
    else if (((e->mode() != gpsdclient::FIX_NONE)) && (utils::dist(pos(), e->pos()) >= 0.001))
        motion = true;
     
    mode(e->mode());
    track(e->track());

    if (motion) 
    {
        valid(true);
        pos(e->pos());
        Fl::awake(cb_fire_event_motion, this);
    }

    return true;
};

void gpsdlayer::draw(const viewport &viewport, canvas &os)
{
    if (!m_gpsdclient)
        return;

    if (!m_gpsdclient->connected())
        return;

    if (!valid())
        return;

    // Calculate cursor rotation
    point2d<int> p1((int)((cos((90.0+track())*(M_PI/180.0))*17.0)), (int)(sin((90.0+track())*(M_PI/180))*17.0));
    point2d<int> p2((int)((cos((222.0+track())*(M_PI/180.0))*17.0)), (int)(sin((222.0+track())*(M_PI/180))*17.0));
    point2d<int> p3((int)((cos((310.0+track())*(M_PI/180.0))*17.0)), (int)(sin((310.0+track())*(M_PI/180))*17.0));

    // Calculate current pixel position on the map
    point2d<unsigned long> pxpos(utils::wsg842px(viewport.z(), pos()));
    pxpos[0] -= viewport.x();
    pxpos[1] -= viewport.y();

    // Draw cursor
    os.fgcolor(color(0xff,0x00,0xb4));
    os.line(pxpos.x()-p1.x(), pxpos.y()-p1.y(), pxpos.x()-p2.x(), pxpos.y()-p2.y(), 2);
    os.line(pxpos.x()-p2.x(), pxpos.y()-p2.y(), pxpos.x(), pxpos.y(), 2);
    os.line(pxpos.x(), pxpos.y(), pxpos.x()-p3.x(), pxpos.y()-p3.y(), 2);
    os.line(pxpos.x()-p3.x(), pxpos.y()-p3.y(), pxpos.x()-p1.x(), pxpos.y()-p1.y(), 2);
};

