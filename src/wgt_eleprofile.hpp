#ifndef WGT_ELEPROFILE_HPP
#define WGT_ELEPROFILE_HPP

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "event.hpp"
#include "gfx.hpp"

class wgt_eleprofile : public Fl_Widget, public event_listener
{
    public:
        wgt_eleprofile(int x, int y, int w, int h, const char *label);
        ~wgt_eleprofile();

        void trackpoints(const std::vector<gpxlayer::waypoint>& wpts);

        // FLTK event handling routine
        int handle(int event);

    private:
        fgfx::canvas m_offscreen;
        std::vector<gpxlayer::waypoint> m_wpts;

        // Utility methods
        void refresh();
        void draw_profile();

    protected:
        void draw();
};

#endif // WGT_ELEPROFILE_HPP

