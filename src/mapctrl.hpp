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

class mapctrl : public Fl_Widget, public layer_observer
{
    public:
        mapctrl(int x, int y, int w, int h, const char *label);
        ~mapctrl();

        void push_layer(layer* l);

        // TODO: review
        virtual int handle(int event);
        virtual void resize(int x, int y, int w, int h);
        void layer_notify();
        point<double> mousegps();
        int zoom();
        void zoom(unsigned int z);
        void layers(std::vector<layer*> &layers);
        void refresh();

    private:
        static const int MSG_WAKEUP = 0;
        static const int MSG_EXIT   = 1;

        point<int> m_mousepos;
        viewport m_viewport;
       canvas m_offscreen;


        std::vector<layer*> m_layers;

    protected:
        void draw();
};

#endif // MAPCTRL_HPP

