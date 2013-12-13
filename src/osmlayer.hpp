#ifndef OSMLAYER_HPP
#define OSMLAYER_HPP

#include <vector>
//#include <ostream>
#include "layer.hpp"
#include "viewport.hpp"
#include "download.hpp"
#include "sqlitecache.hpp"
#include "gfx.hpp"
#include "settings.hpp"

class osmlayer : public layer, public download_observer
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
        typedef struct {
            int x;
            int y;
            int z;
        } tile_t;
        typedef struct {
            tile_t t;
            download *dl;
        } dlref_t;
       
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

        std::vector<dlref_t> m_downloads;
        std::vector<tile_t> m_downloadq;

        bool drawvp(const viewport &viewport, canvas &c);
        void update_map(const viewport &vp);

        static void download_callback(void *data);
        void download_notify(void);
        void download_process(void);
        void download_startnext(void);
        void download_qtile(const tile_t &tile);
};

#endif // OSMLAYER_HPP
