#include "gfx.hpp"
#include <iostream>

namespace fgfx
{
    canvas::canvas(unsigned int w, unsigned int h) :
        m_init(false), 
        m_w(w), 
        m_h(h),
        m_fgcolor(0xffffff),
        m_bgcolor(0x000000)
    {
    };

    canvas::~canvas()
    {
        if (m_init)
        {
            fl_delete_offscreen(m_buf);
        }
    };

    void canvas::fgcolor(color c)
    {
        m_fgcolor = c;
    };

    void canvas::bgcolor(color c)
    {
        m_bgcolor = c;
    };

    color canvas::bgcolor()
    {
        return m_bgcolor;
    };

    color canvas::fgcolor()
    {
        return m_fgcolor;
    }

    void canvas::resize(unsigned int w, unsigned int h)
    {
        m_w = (w > m_w) ? w : m_w;
        m_h = (h > m_h) ? h : m_h;

        if (m_init) 
        {
            fl_delete_offscreen(m_buf);
            m_init = false;
        }

        trycreate();
    };

    void canvas::draw(canvas& src, int srcx, int srcy, int srcw, int srch, int dstx, int dsty)
    {
        trycreate();

        fl_begin_offscreen(m_buf);
        fl_copy_offscreen(dstx, dsty, srcw, srch, src.buf(), srcx, srcy);
        fl_end_offscreen();
    };

    void canvas::draw(image &src, int dstx, int dsty)
    {
        trycreate();

        fl_begin_offscreen(m_buf);

        // PNG alpha blending does not work with negative offsets and automatic
        // clipping so we need to calculate offset and bounding box ourselves
        int offsx = 0, offsy = 0;
        int dw = (w()-dstx), dh = (h()-dsty);
        if (dstx < 0)
        {
            offsx = -dstx;
            dw = src.w() - offsx;
            dstx = 0;
        }
        if (dsty < 0)
        {
            offsy = -dsty;
            dh = src.h() - offsy;
            dsty = 0;
        }

        src.buf()->draw(dstx, dsty, dw, dh, offsx, offsy);
        fl_end_offscreen();
    };

    void canvas::fillrect(int x, int y, int w, int h)
    {
        fl_color(m_fgcolor.r(), m_fgcolor.g(), m_fgcolor.b());
        fl_begin_offscreen(m_buf);
        fl_rectf(x, y, w, h, m_fgcolor.r(), m_fgcolor.g(), m_fgcolor.b());
        fl_end_offscreen();
    };

    void canvas::rect(int x, int y, int w, int h)
    {
        fl_color(m_fgcolor.r(), m_fgcolor.g(), m_fgcolor.b());
        fl_begin_offscreen(m_buf);
        fl_rect(x, y, w, h); 
        fl_end_offscreen();
    }

    void canvas::line(int x1, int y1, int x2, int y2, int linewidth)
    {
        fl_color(m_fgcolor.r(), m_fgcolor.g(), m_fgcolor.b());
        fl_line_style(FL_SOLID, linewidth, NULL);
        fl_begin_offscreen(m_buf);
        fl_line(x1, y1, x2, y2);
        fl_line_style(0);
        fl_end_offscreen();
    };

    void canvas::circle(double x, double y, double r)
    {
        fl_color(m_fgcolor.r(), m_fgcolor.g(), m_fgcolor.b());
        fl_begin_offscreen(m_buf);
        fl_circle(x, y, r);
        fl_end_offscreen();
    };

    void canvas::fontsize(int s)
    {
        fl_begin_offscreen(m_buf);
        fl_font(FL_HELVETICA, s);
        fl_end_offscreen();
    };

    void canvas::text(const std::string & txt, int x, int y)
    {
        fl_begin_offscreen(m_buf);
        fl_draw(txt.c_str(), x, y-fl_descent()+fl_height());
        fl_end_offscreen();
    };

    void canvas::trycreate(void)
    {
        if (m_init)
            return;

        m_buf = fl_create_offscreen(m_w, m_h);
        m_init = true;
    };

    image::image(int type, const unsigned char *buffer, int bufsize) :
        m_type(type),
        m_init(false),
        m_buf(NULL)
    {
        switch (type)
        {
            case PNG:
                {
                    m_buf = new Fl_PNG_Image(NULL, buffer, bufsize);
                    m_init = true;
                    break;
                }
            case JPG:
                {
                    m_buf = new Fl_JPEG_Image(NULL, buffer);
                    m_init = true;
                    break;
                }
            default:
                m_buf = NULL;
                m_init = false;
        }
    };

    image::~image()
    {
        if (m_init)
        {
            delete m_buf;
        }
    };

    int image::w()
    {
        if (m_init)
        {
            return m_buf->w();
        }

        return 0;
    };

    int image::h()
    {
        if (m_init)
        {
            return m_buf->h();
        }

        return 0;
    };

}

