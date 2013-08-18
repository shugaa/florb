#ifndef ELPROCTRL_HPP
#define ELPROCTRL_HPP

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>

class elproctrl : public Fl_Widget
{
    public:
        elproctrl(int x, int y, int w, int h, const char *label);
        ~elproctrl();

        void draw();
        int handle(int event);
        void resize(int x, int y, int w, int h);

    private:

    protected:
};

#endif // ELPROCTRL_HPP

