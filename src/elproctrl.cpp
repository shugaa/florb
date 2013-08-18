// Copyright (c) 2010, Björn Rehm (bjoern@shugaa.de)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <iostream>
#include <FL/fl_draw.H>
#include <FL/x.H>
#include "elproctrl.hpp"

elproctrl::elproctrl(int x, int y, int w, int h, const char *label) : 
    Fl_Widget(x, y, w, h, label)
{
}

elproctrl::~elproctrl()
{
}

int elproctrl::handle(int event) 
{
    switch (event) {
        case FL_MOVE:
            return 1;
        case FL_ENTER:
            fl_cursor(FL_CURSOR_HAND);
            return 1;
        case FL_LEAVE:
            fl_cursor(FL_CURSOR_DEFAULT);
            return 1;
        case FL_PUSH:
            return 1;
        case FL_RELEASE: 
            return 1;
        case FL_DRAG: 
            return 1;
        case FL_MOUSEWHEEL:
            return 1;
    }

    return Fl_Widget::handle(event);
}

void elproctrl::draw() 
{
    // Fill the area which the viewport does not cover
    fl_rectf(x(), y(), w(), h(), 80, 80, 80);

    // Create ancanvas drawing buffer and send all subsequent commands there
//BR     Fl_Offscreencanvas;
//BR    canvas = fl_create_offscreen(m_viewport->w(), m_viewport->h());
//BR     fl_begin_offscreencanvas);
//BR 
//BR     // Background-fill thecanvas buffer (tiles might be missing)
//BR     fl_rectf(0, 0, m_viewport->w(), m_viewport->h(), 80, 80, 80);
//BR 
//BR     // Draw all the layers
//BR     for (orb_stack::iterator iter=stack.begin();iter!=stack.end();++iter)
//BR         (*iter)->draw(*m_viewport);
//BR 
//BR     // Blit the generated viewport bitmap onto the widget (centered)
//BR     int dpx = 0, dpy = 0;
//BR     if (w() > (int)m_viewport->w())
//BR         dpx = (w() - (int)m_viewport->w())/2;
//BR     if (h() > (int)m_viewport->h())
//BR         dpy = (h() - (int)m_viewport->h())/2;
//BR 
//BR     fl_end_offscreen();
//BR     fl_copy_offscreen(x()+dpx, y()+dpy, m_viewport->w(), m_viewport->h(),canvas, 0, 0);
//BR     fl_delete_offscreencanvas);
}

void elproctrl::resize(int x, int y, int w, int h)
{
    Fl_Widget::resize(x, y, w, h);
}

