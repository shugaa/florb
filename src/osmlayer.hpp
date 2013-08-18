#ifndef OSMLAYER_HPP
#define OSMLAYER_HPP

#include <vector>
#include <ostream>
#include "layer.hpp"
#include "viewport.hpp"
#include "download.hpp"
#include "sqlitecache.hpp"
#include "gfx.hpp"

class tileserver
{
    public:
        tileserver() :
            m_separator(" ") {};
        tileserver(std::string name, std::string url, int parallel) : 
            m_separator(" "),
            m_name(name),
            m_url(url),
            m_parallel(parallel) {};
        ~tileserver() {};
        friend std::ostream& operator<< (std::ostream& out, const tileserver& ts);
        friend std::istream &operator>> (std::istream& in, tileserver &ts);
    
        std::string name(void) const { return m_name; };
        void name(std::string s) { m_name = s; };

        std::string url(void) const { return m_url; };
        void url(std::string s) { m_url = s; };

        int parallel(void) const { return m_parallel; };
        void parallel(int n) { m_parallel = n; };

    private:
        std::string m_separator;
        std::string m_name;
        std::string m_url;
        int m_parallel;
};

class osmlayer : public layer, public download_observer
{
    public:
        osmlayer();
        ~osmlayer();
        void draw(const viewport &viewport, canvas &c);
        void numdownloads(unsigned int n) { m_numdownloads = n; };
        unsigned int numdownloads(void) { return m_numdownloads; };

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

        unsigned int m_numdownloads;
        sqlitecache *m_cache;
        viewport m_vp;
        std::vector<char> m_imgbuf;

        std::vector<dlref_t> m_downloads;
        std::vector<tile_t> m_downloadq;

        bool drawvp(const viewport &viewport, canvas &c);
        void update_map(const viewport &vp);

        static void download_callback(void *data);
        void download_notify(void);
        void download_process(bool startnext);
        void download_startnext(void);
        void download_qtile(const tile_t &tile);
};

#endif // OSMLAYER_HPP
