#ifndef GFX_HPP
#define GFX_HPP

#include <FL/x.H>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_JPEG_Image.H>

typedef Fl_Offscreen canvas_storage;
typedef Fl_Image* image_storage;

// Represents any kind of surface that can be drawn on
class drawable
{
    public:
        virtual unsigned int w() = 0;
        virtual unsigned int h() = 0;
        virtual void fillrect(int x, int y, int w, int h) = 0;
        virtual void line(int x1, int y1, int x2, int y2, int linewidth) = 0;
    private: 
};

class color
{
    public:
        color() {};
        color(unsigned char r, unsigned char g, unsigned char b) : 
            m_r(r), 
            m_g(g), 
            m_b(b) {};
        color(int c) :
            m_r((c>>16) & 0xff),
            m_g((c>>8) & 0xff),
            m_b(c & 0xff) {};
        color(const color& c) :
            m_r(c.r()),
            m_g(c.g()),
            m_b(c.b()) {};

        ~color() {};

        unsigned char r() const { return m_r; };
        unsigned char g() const { return m_g; };
        unsigned char b() const { return m_b; };

        unsigned int rgb() const 
        {
            unsigned int ret = 0;
            ret |= m_r; ret <<= 8;
            ret |= m_g; ret <<= 8;
            ret |= m_b;
            return ret;
        }

    private:
        unsigned char m_r;
        unsigned char m_g;
        unsigned char m_b;
};

class image
{
    public:
        image(int type, const unsigned char *buffer, int bufsize);
        ~image();

        image_storage buf(void) { return m_buf; };
        int type() { return m_type; };
        void buf(image_storage bufs) { m_buf = bufs; };

        enum {
            PNG,
            JPG
        };

    private:
        int m_type;
        bool m_init;
        image_storage m_buf;
};

// An in-memory drawing buffer 
class canvas : public drawable
{
    public:
        canvas(unsigned int w, unsigned int h);
        ~canvas();

        void fgcolor(color fg);
        void bgcolor(color bg);
        color fgcolor();
        color bgcolor();
        void resize(unsigned int w, unsigned int h);
        void draw(canvas& src, int srcx, int srcy, int srcw, int srch, int dstx, int dsty);
        void draw(image &src, int dstx, int dsty);
        void fillrect(int x, int y, int w, int h);
        void line(int x1, int y1, int x2, int y2, int linewidth);
        canvas_storage buf(void) { trycreate(); return m_buf; };
        void buf(canvas_storage bufs) { m_buf = bufs; };
        unsigned int w(void) { return m_w; };
        unsigned int h(void) { return m_h; };
        void w(unsigned int ws) { m_w = ws; };
        void h(unsigned int hs) { m_h = hs; };

    private:
        bool m_init;
        unsigned int m_w;
        unsigned int m_h;
        canvas_storage m_buf;

        color m_fgcolor;
        color m_bgcolor;

        void trycreate(void);
};

#endif // GFX_HPP
