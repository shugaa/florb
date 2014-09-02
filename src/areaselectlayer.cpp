#include <cmath>
#include <sstream>
#include "gfx.hpp"
#include "utils.hpp"
#include "point.hpp"
#include "settings.hpp"
#include "areaselectlayer.hpp"

areaselectlayer::areaselectlayer() :
    m_p1(-1,-1),
    m_p2(-1,-1)
{
    name(std::string("AreaSelect"));
    register_event_handler<areaselectlayer, florb::layer::event_mouse>(this, &areaselectlayer::handle_evt_mouse);
};

areaselectlayer::~areaselectlayer()
{
};

void areaselectlayer::clear()
{
    m_p1[0] = -1;
    m_p1[1] = -1;
    m_p2[0] = -1;
    m_p2[1] = -1;

    notify();
}

bool areaselectlayer::handle_evt_mouse(const florb::layer::event_mouse* evt)
{
    if (!enabled())
    {
        return false;
    }

    // Only the left mouse button is of interest
    if (evt->button() != florb::layer::event_mouse::BUTTON_LEFT)
        return false;

    bool ret = false;
    switch (evt->action())
    {
        case florb::layer::event_mouse::ACTION_PRESS:
        {
            ret = press(evt); 
            break;
        }
        case florb::layer::event_mouse::ACTION_RELEASE:
        {
            ret = release(evt); 
            break;
        }
        case florb::layer::event_mouse::ACTION_DRAG:
        {
            ret = drag(evt); 
            break;
        }
        default:
            ;
    }

    return ret;
}

bool areaselectlayer::press(const florb::layer::event_mouse* evt)
{
    m_p1[0] = evt->pos().x() < 0 ? 0 : (unsigned long)evt->pos().x();
    m_p1[1] = evt->pos().y() < 0 ? 0 : (unsigned long)evt->pos().y();

    // Mouse push outside viewport area
    if (m_p1.x() >= evt->vp().w())
        m_p1[0] = evt->vp().w()-1;
    if (m_p1.y() >= evt->vp().h())
        m_p1[1] = evt->vp().h()-1;

    return true;
}

bool areaselectlayer::drag(const florb::layer::event_mouse* evt)
{
    florb::point2d<unsigned long> px(evt->pos().x(), evt->pos().y());
    
    // Catch drag outside map area
    if (evt->pos().x() < 0)
        px[0] = 0;
    else if (evt->pos().x() >= (int)evt->vp().w())
        px[0] = evt->vp().w()-1;
    if (evt->pos().y() < 0)
        px[1] = 0;
    else if (evt->pos().y() >= (int)evt->vp().h())
        px[1] = evt->vp().h()-1;

    m_p2 = px;

    // Trigger redraw
    notify();

    return true;
}

bool areaselectlayer::release(const florb::layer::event_mouse* evt)
{
    m_vp = viewport(
        evt->vp().x() + ((m_p1.x() > m_p2.x()) ? m_p2.x() : m_p1.x()),
        evt->vp().y() + ((m_p1.y() > m_p2.y()) ? m_p2.y() : m_p1.y()),
        evt->vp().z(),
        (m_p1.x() > m_p2.x()) ? m_p1.x()-m_p2.x() : m_p2.x()-m_p1.x(),
        (m_p1.y() > m_p2.y()) ? m_p1.y()-m_p2.y() : m_p2.y()-m_p1.y());
        
    done();

    return true;
}

void areaselectlayer::done()
{
    event_done e(m_vp);
    fire(&e);
}

void areaselectlayer::notify()
{
    event_notify e;
    fire(&e);
}

bool areaselectlayer::draw(const viewport &viewport, florb::canvas &os)
{
    if (m_p1.x() < 0)
        return true;
    if (m_p2.x() < 0)
        return true;
    if (m_p1.y() < 0)
        return true;
    if (m_p2.y() < 0)
        return true;

    // TODO: Performance killer!!
    florb::cfg_ui cfgui = florb::settings::get_instance()["ui"].as<florb::cfg_ui>();
    
    os.fgcolor(cfgui.selectioncolor());
    os.line(m_p1.x(), m_p1.y(), m_p2.x(), m_p1.y(), 1);
    os.line(m_p2.x(), m_p1.y(), m_p2.x(), m_p2.y(), 1);
    os.line(m_p2.x(), m_p2.y(), m_p1.x(), m_p2.y(), 1);
    os.line(m_p1.x(), m_p2.y(), m_p1.x(), m_p1.y(), 1);

    return true;
};

