#ifndef OSMLAYER_HPP
#define OSMLAYER_HPP

#include <vector>
#include "layer.hpp"
#include "viewport.hpp"
#include "downloader.hpp"
#include "sqlitecache.hpp"
#include "gfx.hpp"
#include "settings.hpp"

class osmlayer : public layer
{
    public:
        osmlayer(
            const std::string& nm,  
            const std::string& url, 
            unsigned int zmin,
            unsigned int zmax,
            unsigned int parallel,
            int imgtype);
        ~osmlayer();
        void draw(const viewport &vp, canvas &c);

        int zoom_min() { return m_zmin; };
        int zoom_max() { return m_zmax; };

        class event_notify;

    private:
        class tileinfo;

        canvas m_canvas_0;
        canvas m_canvas_1;
        canvas m_canvas_tmp;

        std::string m_name;
        std::string m_url;
        unsigned int m_zmin;
        unsigned int m_zmax;
        unsigned int m_parallel;
        int m_type;
        
        sqlitecache *m_cache;
        viewport m_vp;
        std::vector<char> m_imgbuf;

        static void cb_download(void *userdata);
        std::vector<tileinfo*> m_tileinfos;
        void process_downloads();

        downloader* m_downloader;

        bool drawvp(const viewport &viewport, canvas &c);
        void update_map(const viewport &vp);

        void download_qtile(int z, int x, int y);

        bool evt_downloadcomplete(const downloader::event_complete *e);

        bool m_test;
        static void testcb(void *ud);
};

class osmlayer::event_notify : public event_base
{
    public:
        event_notify() {};
        ~event_notify() {};
};

#endif // OSMLAYER_HPP

