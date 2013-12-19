#include <cmath>
#include <sstream>
#include "gfx.hpp"
#include "utils.hpp"
#include "settings.hpp"
#include "point.hpp"
#include "gpsdlayer.hpp"

gpsdlayer::gpsdlayer() :
    layer(),
    m_gpsdclient(NULL)
{
    name(std::string("Unnamed GPSD layer"));

    m_gpsinfo.valid = false;
    m_gpsdclient = new gpsdclient("localhost", "2947");

    if (m_gpsdclient)
        m_gpsdclient->add_event_listener(this);

    register_event_handler<gpsdlayer, gpsdclient_update_event>(this, &gpsdlayer::handle_evt_gpsupdate);
    //register_event_handler<gpsdlayer, gpsdclient_status_event>(this, &gpsdlayer::handle_evt_status);
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

bool gpsdlayer::handle_evt_gpsupdate(const gpsdclient_update_event *e)
{
    m_gpsinfo.pos = point2d<double>(e->longitude, e->latitude);
    m_gpsinfo.track = e->track;
    m_gpsinfo.mode = e->mode;
    
    m_gpsinfo.valid = true;

    notify_observers();
    return true;
};

//bool gpsdlayer::handle_evt_status(const gpsdclient_status_event *e)
//{
//    m_connected = e->connected;
//    notify_observers();
//    return true;
//}

void gpsdlayer::draw(const viewport &viewport, canvas &os)
{
    if (!m_gpsdclient)
        return;

    if (!m_gpsdclient->connected())
        return;

    if (m_gpsinfo.valid == false)
        return;

    // Calculate cursor rotation
    point2d<int> p1((int)((cos((90.0+m_gpsinfo.track)*(M_PI/180.0))*15.0)), (int)(sin((90.0+m_gpsinfo.track)*(M_PI/180))*15.0));
    point2d<int> p2((int)((cos((222.0+m_gpsinfo.track)*(M_PI/180.0))*15.0)), (int)(sin((222.0+m_gpsinfo.track)*(M_PI/180))*15.0));
    point2d<int> p3((int)((cos((310.0+m_gpsinfo.track)*(M_PI/180.0))*15.0)), (int)(sin((310.0+m_gpsinfo.track)*(M_PI/180))*15.0));

    // Calculate current pixel position on the map
    point2d<unsigned long> pxpos;
    utils::gps2px(viewport.z(), m_gpsinfo.pos, pxpos);
    pxpos[0] -= viewport.x();
    pxpos[1] -= viewport.y();

    // Draw cursor
    os.fgcolor(color(255,0,0));
    os.line(pxpos.x()-p1.x(), pxpos.y()-p1.y(), pxpos.x()-p2.x(), pxpos.y()-p2.y(), 2);
    os.line(pxpos.x()-p2.x(), pxpos.y()-p2.y(), pxpos.x(), pxpos.y(), 2);
    os.line(pxpos.x(), pxpos.y(), pxpos.x()-p3.x(), pxpos.y()-p3.y(), 2);
    os.line(pxpos.x()-p3.x(), pxpos.y()-p3.y(), pxpos.x()-p1.x(), pxpos.y()-p1.y(), 2);
};

