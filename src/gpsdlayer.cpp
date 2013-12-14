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
    m_gpsdclient = new gpsdclient("localhost", "2947");

    if (m_gpsdclient)
        m_gpsdclient->addobserver(*this);
};

gpsdlayer::~gpsdlayer()
{
    if (m_gpsdclient)
        delete m_gpsdclient;
};

void gpsdlayer::gpsdclient_notify(void)
{
    Fl::awake(gpsdlayer::gpsdclient_callback, (void*)this);
};

void gpsdlayer::gpsdclient_callback(void *data)
{
    gpsdlayer *m = reinterpret_cast<gpsdlayer*>(data);
    m->gpsdclient_process();
};

void gpsdlayer::gpsdclient_process(void)
{
    std::cout << "gpsd notify" << std::endl;
    notify_observers();
};

void gpsdlayer::draw(const viewport &viewport, canvas &os)
{
    if (!m_gpsdclient)
        return;
    //if (m_gpsdclient->mode() == gpsdclient::FIX_NONE)
    //    return;

    point2d<double> gpspos(m_gpsdclient->longitude(), m_gpsdclient->latitude());
    point2d<unsigned long> pxpos;

    utils::gps2px(viewport.z(), gpspos, pxpos);
    os.fgcolor(color(0,0,255));
    os.fillrect((pxpos.x()-viewport.x())-5, (pxpos.y()-viewport.y())-5, 10, 10);
};

