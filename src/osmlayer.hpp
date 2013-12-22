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

    private:
        class tileinfo 
        {
            public:
                int x;
                int y;
                int z;
        };
       
        canvas m_canvas_0;
        canvas m_canvas_1;
        canvas m_canvas_tmp;

        bool m_shutdown;

        std::string m_name;
        std::string m_url;
        unsigned int m_zmin;
        unsigned int m_zmax;
        unsigned int m_parallel;
        int m_type;
        
        sqlitecache *m_cache;
        viewport m_vp;
        std::vector<char> m_imgbuf;

        std::vector<downloader*> m_downloaders;
        std::vector<tileinfo> m_downloadq;

        bool drawvp(const viewport &viewport, canvas &c);
        void update_map(const viewport &vp);

        void download_process(void);
        void download_startnext(void);
        void download_qtile(const tileinfo& tile);

        bool evt_downloadcomplete(const downloader::event_complete *e);

        bool m_test;
        static void testcb(void *ud);
};

#endif // OSMLAYER_HPP
