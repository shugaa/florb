#ifndef OSMLAYER_HPP
#define OSMLAYER_HPP

#include <vector>
#include "layer.hpp"
#include "viewport.hpp"
#include "downloader.hpp"
#include "cache.hpp"
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
        bool draw(const viewport &vp, florb::canvas &c);
        bool download(const viewport& vp, double& coverage);
        void nice(long ms);

        int zoom_min() { return m_zmin; };
        int zoom_max() { return m_zmax; };
        void dlenable(bool e);

        static const std::string wcard_x;
        static const std::string wcard_y;
        static const std::string wcard_z;

        class event_notify;

    private:
        static const char tile_empty[];

        class tileinfo;

        std::string m_name;
        std::string m_url;
        unsigned int m_zmin;
        unsigned int m_zmax;
        unsigned int m_parallel;
        int m_type;
        
        cache *m_cache;
        std::vector<char> m_imgbuf;
        std::vector<tileinfo*> m_tileinfos;
        downloader* m_downloader;
        bool m_dlenable;

        static void cb_download(void *userdata);
        void process_downloads();

        bool drawvp(const viewport &viewport, florb::canvas *c, unsigned long *ttotal, unsigned long *tnok);
        void download_qtile(int z, int x, int y);
        bool evt_downloadcomplete(const downloader::event_complete *e);
};

class osmlayer::event_notify : public event_base
{
    public:
        event_notify() {};
        ~event_notify() {};
};

#endif // OSMLAYER_HPP

