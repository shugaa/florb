#ifndef MAPCTRL_HPP
#define MAPCTRL_HPP

#include <vector>
#include <string>
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "viewport.hpp"
#include "point.hpp"
#include "layer.hpp"
#include "gfx.hpp"

#include "download.hpp"

class mapctrl_observer;

class mapctrl : public Fl_Widget, public layer_observer
{
    public:
        mapctrl(int x, int y, int w, int h, const char *label);
        ~mapctrl();

        void basemap(
                const std::string& name, 
                const std::string& url, 
                unsigned int zmin, 
                unsigned int zmax, 
                unsigned int parallel,
                int imgtype);

        int handle(int event);
        void layer_notify();
        void refresh();

        point2d<double> mousegps();
        unsigned int zoom();
        void zoom(unsigned int z);

        void addobserver(mapctrl_observer &o);
        void removeobserver(mapctrl_observer &o);

    private:
        //static const int MSG_WAKEUP = 0;
        //static const int MSG_EXIT   = 1;

        layer *m_basemap;
        layer *m_gpxlayer;
        layer *m_gpsdlayer;

        point2d<int> m_mousepos;
        viewport m_viewport;
        canvas m_offscreen;

        std::set<mapctrl_observer*> m_observers;
        void notify_observers();

    protected:
        void draw();
};

class mapctrl_observer
{
    public:
        virtual void mapctrl_notify() = 0;
};

#endif // MAPCTRL_HPP

