#include <cmath>
#include "utils.hpp"
#include "osmlayer.hpp"

#define ONE_WEEK                (7*24*60*60)
#define ONE_DAY                 (1*24*60*60)
#define TILE_W                  (256)
#define TILE_H                  (256)

const std::string osmlayer::wcard_x = "{x}";
const std::string osmlayer::wcard_y = "{y}";
const std::string osmlayer::wcard_z = "{z}";

const char osmlayer::tile_empty[] = \
	"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A\x00\x00\x00\x0D\x49\x48\x44\x52\x00\x00\x01\x00" \
	"\x00\x00\x01\x00\x08\x06\x00\x00\x00\x5C\x72\xA8\x66\x00\x00\x00\x06\x62\x4B\x47" \
	"\x44\x00\xFF\x00\xFF\x00\xFF\xA0\xBD\xA7\x93\x00\x00\x00\x09\x70\x48\x59\x73\x00" \
	"\x00\x0B\x13\x00\x00\x0B\x13\x01\x00\x9A\x9C\x18\x00\x00\x01\x15\x49\x44\x41\x54" \
	"\x78\xDA\xED\xC1\x31\x01\x00\x00\x00\xC2\xA0\xF5\x4F\xED\x6B\x08\xA0\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" \
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x78\x03\x01\x3C\x00\x01\xD8\x29\x43" \
	"\x04\x00\x00\x00\x00\x49\x45\x4E\x44\xAE\x42\x60\x82";

class osmlayer::tileinfo
{
    public:
        tileinfo(int z, int x, int y) :
            m_z(z),
            m_x(x),
            m_y(y) {};

        int z() const { return m_z; };
        int x() const { return m_x; };
        int y() const { return m_y; };

    private:
        int m_z;
        int m_x;
        int m_y;
};

osmlayer::osmlayer(
        const std::string& nm,  
        const std::string& url, 
        unsigned int zmin,
        unsigned int zmax,
        unsigned int parallel,
        int imgtype) :
    layer(),                        // Base class constructor 
    m_name(nm),                     // Layer name
    m_url(url),                     // Tileserver URL
    m_zmin(zmin),                   // Min. zoomlevel supported by server
    m_zmax(zmax),                   // Max. zoomlevel supported by server
    m_parallel(parallel),           // Number of simultaneous downloads
    m_type(imgtype),                // Tile image data type
    m_dlenable(true)                // Allow tile downloading
{
    // Set map layer name
    name(m_name);

    // Create cache
    cfg_cache cfgcache = settings::get_instance()["cache"].as<cfg_cache>();
    try {
        m_cache = new sqlitecache(cfgcache.location());
    } catch (std::runtime_error& e) {
        throw e;
    }
    // Create a cache session for the given URL
    try {
        m_cache->sessionid(url);
    } catch (std::runtime_error& e) {
        delete m_cache;
        throw e;
    }

    // Create the requested number of download threads
    try {
        m_downloader = new downloader(parallel);
    } catch (std::runtime_error& e) {
        delete m_cache;
        throw e;
    }

    // Register event handlers
    register_event_handler<osmlayer, downloader::event_complete>(this, &osmlayer::evt_downloadcomplete);
    m_downloader->add_event_listener(this);
};

osmlayer::~osmlayer()
{
    // destroy the downloader
    delete m_downloader;

    // Destroy all download userdata
    std::vector<tileinfo*>::iterator it;
    for (it=m_tileinfos.begin();it!=m_tileinfos.end();++it)
    {
        delete (*it);
    }

    // Destroy the cache
    delete m_cache;
};

void osmlayer::process_downloads()
{
    bool ret = false; 

    // Cache all downloaded tiles
    downloader::download dtmp;
    while (m_downloader->get(dtmp))
    {
        std::vector<tileinfo*>::iterator it = 
            std::find(m_tileinfos.begin(), m_tileinfos.end(), dtmp.userdata());

        tileinfo *ti;
        if (it == m_tileinfos.end())
        {
            continue; 
        } 
        else
        {
            ti = (*it);
            m_tileinfos.erase(it);
        }

        // If the download was not successful, we set the expiration date to
        // one day in the future to retry the download in a timely fashion.
        time_t expires = (dtmp.buf().size() != 0) ? 
            dtmp.expires() : ONE_DAY;
        time_t now = time(NULL);

        // We probably didn't get a valid expires header
        if (expires <= now)
            expires = now + ONE_WEEK;

        ret = true;

        try {
            m_cache->put(ti->z(), ti->x(), ti->y(), expires, dtmp.buf());
        } catch (std::runtime_error& e) {
            ret = false;
        }

        delete ti;
    }

    if (ret) 
    {
        event_notify e;
        fire(&e);
    }
}

void osmlayer::dlenable(bool e)
{
    m_dlenable = e;
    if (e)
    {
        //Kickstart the downloader
        event_notify ev;
        fire(&ev);
    }
}

void osmlayer::cb_download(void *userdata)
{
    // This might be a callback for an already destroyed layer instance
    layer *l = static_cast<layer*>(userdata);
    if (!is_instance(l))
        return;

    osmlayer *osml = dynamic_cast<osmlayer*>(l);
    if (!osml)
        return;

    osml->process_downloads();
}

bool osmlayer::evt_downloadcomplete(const downloader::event_complete *e)
{
    Fl::awake(cb_download, this);
    return true;
}

void osmlayer::download_qtile(int z, int x, int y)
{
    if (!m_dlenable)
        return;

    // Check whether the requested tile is already being processed
    std::vector<tileinfo*>::iterator it;
    for (it=m_tileinfos.begin();it!=m_tileinfos.end();++it)
    {
        if (((*it)->z() == z) && ((*it)->x() == x) && ((*it)->y() == y))
            return;
    }

    // Create the userdata component to be attached to the download
    tileinfo *ti = new tileinfo(z, x, y);

    // Construct the download URL
    std::ostringstream sz, sx, sy;
    sz << z;
    sx << x;
    sy << y;

    std::string url(m_url);
    
    std::size_t idx = url.find(osmlayer::wcard_z);
    if (idx != std::string::npos)
        url.replace(idx, osmlayer::wcard_z.length(), sz.str());

    idx = url.find(osmlayer::wcard_x);
    if (idx != std::string::npos)
        url.replace(idx, osmlayer::wcard_x.length(), sx.str());

    idx = url.find(osmlayer::wcard_y);
    if (idx != std::string::npos)
        url.replace(idx, osmlayer::wcard_y.length(), sy.str());

    // Try to queue this URL for downloading
    bool ret = m_downloader->queue(url, ti);

    // Item queued for downloading
    if (ret)
    {
        m_tileinfos.push_back(ti); 
    }
    // Item not added
    else
        delete ti;
}

bool osmlayer::draw(const viewport &vp, fgfx::canvas &os)
{
    if ((vp.z() < m_zmin) || (vp.z() > m_zmax))
    {
        // Zoomlevel not supported by this tile layer
        return true;
    } 

    // Regular tile drawing
    return drawvp(vp, os);
}

bool osmlayer::drawvp(const viewport &vp, fgfx::canvas &c)
{
    // Return whether the map image has tiles missing (false) or not (true)
    bool ret = true;

    // Get the x and y start tile index
    unsigned int tstartx = vp.x() / TILE_W;
    unsigned int tstarty = vp.y() / TILE_H;
    
    // If the vp does not exactly hit the tile edge, there is an offset
    // for drawing the individual tiles.
    int pstartx = -((int)(vp.x() % TILE_W));
    int pstarty = -((int)(vp.y() % TILE_H));

    // Start at offset and draw up to vp.w() and vp.h()
    int px, py;
    unsigned int tx, ty;

    for (py=pstarty, ty=tstarty; py<(int)vp.h(); py+=TILE_W, ty++)
    {
       for (px=pstartx, tx=tstartx; px<(int)vp.w(); px+=TILE_H, tx++)
       {
          // Get the tile
          int rc;
          try {
            rc = m_cache->get(vp.z(), tx, ty, m_imgbuf);
          } catch (std::runtime_error& e) {
            rc = sqlitecache::NOTFOUND;
          }
          
          // Draw the tile if we either have a valid or expired version of it...
          if ((rc != sqlitecache::NOTFOUND) && (m_imgbuf.size() != 0))
          {
              fgfx::image img(m_type, (unsigned char*)(&m_imgbuf[0]), m_imgbuf.size());
              c.draw(img, px, py);
          }

          // Tile not in cache or expired, schedule for downloading
          if ((rc == sqlitecache::EXPIRED) || 
              (rc == sqlitecache::NOTFOUND))
          {
              download_qtile(vp.z(), tx, ty);
              ret = false;
          }
       }
    }

    return ret;
}

