#include <cmath>
#include <sstream>
#include "gfx.hpp"
#include "utils.hpp"
#include "settings.hpp"
#include "point.hpp"
#include "markerlayer.hpp"

markerlayer::markerlayer()
{
    name(std::string("Marker"));
};

markerlayer::~markerlayer()
{
};

size_t markerlayer::add(const point2d<double> &pmerc)
{
    size_t i = 0;
    for (;i<m_markers.size();i++)
    {
        bool found = false;

        std::vector<marker_internal>::iterator it;
        for (it=m_markers.begin();it!=m_markers.end();++it)
        {
            if ((*it).id == i)
            {
                found = true;
                break;
            }
        }

        if (!found)
            break;
    }

    add(pmerc, i);
    return i;
};

void markerlayer::add(const point2d<double> &pmerc, size_t id)
{
    marker_internal tmp;
    tmp.p = pmerc;
    tmp.id = id;

    m_markers.push_back(tmp);
    notify();
};

void markerlayer::remove(size_t id)
{
    std::vector<marker_internal>::iterator it;
    for (it=m_markers.begin();it!=m_markers.end();++it)
    {
        if ((*it).id == id)
        {
            it = --(m_markers.erase(it));
        }
    } 

    notify();
};

void markerlayer::clear() 
{
    m_markers.clear();
    notify();
};

void markerlayer::notify()
{
    event_notify e;
    fire(&e);
}

void markerlayer::draw(const viewport &viewport, canvas &os)
{
    cfg_ui cfgui = settings::get_instance()["ui"].as<cfg_ui>();

    std::vector<marker_internal>::iterator it;
    for (it=m_markers.begin();it!=m_markers.end();++it)
    {
        point2d<unsigned long> ppx(utils::merc2px(viewport.z(), (*it).p));

        if (ppx.x() < viewport.x())
            continue;
        if (ppx.x() >= (viewport.x()+viewport.w()))
            continue;
        if (ppx.y() < viewport.y())
            continue;
        if (ppx.y() >= (viewport.y()+viewport.h()))
            continue;

        ppx[0] -= viewport.x();
        ppx[1] -= viewport.y();

        os.fgcolor(cfgui.gpscursorcolor());
        os.line(ppx.x()-12, ppx.y(), ppx.x()+12, ppx.y(), 1);
        os.line(ppx.x(), ppx.y()-12, ppx.x(), ppx.y()+12, 1);

        os.circle(ppx.x(), ppx.y(), 5);
        os.circle(ppx.x(), ppx.y(), 6);
    }
};

