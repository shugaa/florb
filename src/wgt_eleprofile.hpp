#ifndef WGT_ELEPROFILE_HPP
#define WGT_ELEPROFILE_HPP

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "event.hpp"
#include "gfx.hpp"

class wgt_eleprofile : public Fl_Widget, public event_generator
{
    public:
        wgt_eleprofile(int x, int y, int w, int h, const char *label);
        ~wgt_eleprofile();

        void trackpoints(const std::vector<gpxlayer::waypoint>& wpts);

        // FLTK event handling routine
        int handle(int event);
        int handle_move(int event);
        int handle_enter(int event);
        int handle_leave(int event);

        void notify_mouse(double trip, double ele);

        class event_mouse;
    private:
        fgfx::canvas m_offscreen;
        std::vector< point2d<double> > m_wpts;
        double m_elemin;
        double m_elemax;

        // Utility methods
        void refresh();
        void draw_profile();

    protected:
        void draw();
};

class wgt_eleprofile::event_mouse : public event_base
{
    public:
        event_mouse(double trip, double ele) :
            m_trip(trip),
            m_ele(ele)
            {};
        ~event_mouse() {};

        double trip() const { return m_trip; };
        double ele() const { return m_ele; };

    private:
        double m_trip;
        double m_ele;
};

#endif // WGT_ELEPROFILE_HPP

