#include "gfx.hpp"

canvas::canvas(unsigned int w, unsigned int h) :
    m_init(false), m_w(w), m_h(h)
{
};

canvas::~canvas()
{
    if (m_init)
    {
        fl_delete_offscreen(m_buf);
    }
};

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

void canvas::fillrect(int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
{
    fl_begin_offscreen(m_buf);
    fl_rectf(x, y, w, h, r, g, b);
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

