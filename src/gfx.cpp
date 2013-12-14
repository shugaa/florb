#include "gfx.hpp"

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
    src.buf()->draw(dstx, dsty);
    fl_end_offscreen();
};

void canvas::fillrect(int x, int y, int w, int h)
{
    fl_color(m_fgcolor.r(), m_fgcolor.g(), m_fgcolor.b());
    fl_begin_offscreen(m_buf);
    fl_rectf(x, y, w, h, m_fgcolor.r(), m_fgcolor.g(), m_fgcolor.b());
    fl_end_offscreen();
};

void canvas::line(int x1, int y1, int x2, int y2, int linewidth)
{
    fl_color(m_fgcolor.r(), m_fgcolor.g(), m_fgcolor.b());
    fl_line_style(FL_SOLID, linewidth, NULL);
    fl_begin_offscreen(m_buf);
    fl_line(x1, y1, x2, y2);
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

