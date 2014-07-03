#include <cmath>
#include "utils.hpp"
#include "osmlayer.hpp"

#define ONE_WEEK                (7*24*60*60)
#define TILE_W                  (256)
#define TILE_H                  (256)

const std::string osmlayer::wcard_x = "{x}";
const std::string osmlayer::wcard_y = "{y}";
const std::string osmlayer::wcard_z = "{z}";

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
    m_canvas_0(500, 500),           // Canvas for 'double buffering'. Will be resized as needed
    m_canvas_1(500, 500),           // Canvas for 'double buffering'. Will be resized as needed
    m_canvas_tmp(500,500),          // Temporary drawing canvas. Will be resized as needed
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
    m_cache = new sqlitecache(cfgcache.location());

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

        time_t expires = dtmp.expires();
        time_t now = time(NULL);

        if (expires <= now)
            expires = now + ONE_WEEK;

        if (dtmp.buf().size() != 0)
        {
            ret = true;

            try {
                m_cache->put(ti->z(), ti->x(), ti->y(), expires, dtmp.buf());
            } catch (std::runtime_error& e) {
                ret = false;
            }
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

void osmlayer::draw(const viewport &vp, canvas &os)
{
    // Zoomlevel not supported by this tile layer
    if ((vp.z() < m_zmin) || (vp.z() > m_zmax))
    {
        // Simple white background. We might need to do something a little more
        // sophisticated in the future.
        m_canvas_0.resize(vp.w(), vp.h());
        m_canvas_0.fgcolor(color(255,255,255));
        m_canvas_0.fillrect(0, 0, (int)vp.w(), (int)vp.h());
        os.draw(m_canvas_0, 0, 0, (int)vp.w(), (int)vp.h(), 0, 0);

        // Make sure the map is redrawn on zoomlevel change by invalidating the
        // current local viewport
        m_vp.w(0);
        m_vp.h(0);
    } else
    {
        // Regular tile drawing
        update_map(vp);
        os.draw(m_canvas_0, 0, 0, (int)vp.w(), (int)vp.h(), 0, 0);
    }
}

void osmlayer::update_map(const viewport &vp)
{
   // the current layer view is dirty if it is incomplete (tiles are missing or
   // outdated)
   static bool dirty = true;

   // Make sure the new viewport is any different from the last one before
   // going through the hassle of drawing the map anew.
   if ((m_vp != vp) || (dirty))
   {
     // Check if the old and new viewport intersect and eventually recycle any
     // portion of the map image
     viewport vp_inters(vp);
     vp_inters.intersect(m_vp);

     // Old and new viewport intersect, recycle old canvas map image buffer
     if (((vp_inters.w() > 0) && (vp_inters.h() > 0)) && (!dirty))
     {
       // Resize the drawing buffer if it is too small
       if ((m_canvas_1.w() < vp.w()) || (m_canvas_1.h() < vp.h()))
           m_canvas_1.resize(vp.w(), vp.h());

       m_canvas_1.draw(
               m_canvas_0, 
               vp_inters.x() - m_vp.x(), 
               vp_inters.y() - m_vp.y(), 
               vp_inters.w(), 
               vp_inters.h(), 
               vp_inters.x() - vp.x(), 
               vp_inters.y() - vp.y());

       // Draw any portion of the map image not covered by the old image
       // buffer. Effectively, the rectangles left, right, above and below the
       // intersection are drawn.
       for (int i=0;i<4;i++) 
       {
         unsigned long x_tmp, y_tmp, w_tmp, h_tmp;

         // Determine the respective rectangle coordinates
         switch (i)
         {
           // Rectangle to the left if there is one
           case 0:
             if (vp_inters.x() <= vp.x())
             {
               continue;
             }
             x_tmp = vp.x();
             y_tmp = vp.y();
             w_tmp = vp_inters.x() - vp.x();
             h_tmp = vp.h();
             break;
           // Rectangle to the right if there is one
           case 1:
             if ((vp_inters.x() + vp_inters.w()) >= (vp.x() + vp.w()))
             {
               continue;
             }
             x_tmp = vp_inters.x() + vp_inters.w();
             y_tmp = vp.y();
             w_tmp = (vp.x() + vp.w()) - (vp_inters.x() + vp_inters.w());
             h_tmp = vp.h();
             break;
           // Rectangle above if there is one
           case 2:
             if (vp_inters.y() <= vp.y())
             {
               continue;
             }
             x_tmp = vp_inters.x();
             y_tmp = vp.y();
             w_tmp = vp_inters.w();
             h_tmp = vp_inters.y() - vp.y();
             break;
           // Rectangle below if there is one
           case 3:
             if ((vp_inters.y() + vp_inters.h()) >= (vp.y() + vp.h()))
             {
               continue;
             }
             x_tmp = vp_inters.x();
             y_tmp = vp_inters.y() + vp_inters.h();
             w_tmp = vp_inters.w();
             h_tmp = (vp.y() + vp.h()) - (vp_inters.y() + vp_inters.h());
             break;
           default:
             ;
         }

         // Construct a temporary viewport for the respective rectangle
         viewport vp_tmp(x_tmp, y_tmp, vp.z(), w_tmp, h_tmp);

         // Resize the drawing buffer if it is too small
         if ((m_canvas_tmp.w() < vp_tmp.w()) || (m_canvas_tmp.h() < vp_tmp.h()))
             m_canvas_tmp.resize(vp_tmp.w(), vp_tmp.h());

         // Draw the rectangle
         if (!drawvp(vp_tmp, m_canvas_tmp))
            dirty = true;

         // Copy the dummy canvas image into the new map buffer
         m_canvas_1.draw(
            m_canvas_tmp, 
            0, 0, 
            vp_tmp.w(), vp_tmp.h(), 
            vp_tmp.x() - vp.x(), vp_tmp.y() - vp.y());
       }

       // Make the new canvas buffer the current one. We can't just copy
       // construct a temporary canvas object because it would free the
       // internal buffer when it goes out of scope. So we have to switch all
       // members manually here. 
       canvas_storage buftmp = m_canvas_0.buf();
       unsigned int wtmp = m_canvas_0.w();
       unsigned int htmp = m_canvas_0.h();

       m_canvas_0.buf(m_canvas_1.buf());
       m_canvas_0.w(m_canvas_1.w());
       m_canvas_0.h(m_canvas_1.h());

       m_canvas_1.buf(buftmp);
       m_canvas_1.w(wtmp);
       m_canvas_1.h(htmp);
     }
     else
     {
       // Draw the entire map area and make the resulting canvas buffer the
       // current one, it will have the required size (viewport size).
       if ((m_canvas_0.w() < vp.w()) || (m_canvas_0.h() < vp.h()))
           m_canvas_0.resize(vp.w(), vp.h());

       dirty = !drawvp(vp, m_canvas_0);
     }

     /* Save the new viewport for later reference */
     m_vp = vp;
   }
}

bool osmlayer::drawvp(const viewport &vp, canvas &c)
{
    // Return whether the map images has tiles missing (false) or not (true)
    bool ret = true;
    unsigned int parallel = 0;

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
          if (rc != sqlitecache::NOTFOUND)
          {
              image img(m_type, (unsigned char*)(&m_imgbuf[0]), m_imgbuf.size());
              c.draw(img, px, py);
          }
          // ...otherwise draw placeholder image
          else
          {
              c.fgcolor(color(200, 113, 113));
              c.fillrect(px, py, TILE_W, TILE_H);
          }

          // Tile not in cache or expired, schedule for downloading
          if ((rc == sqlitecache::EXPIRED) || 
              (rc == sqlitecache::NOTFOUND))
          {
              //if (parallel < m_parallel)
              {
                  download_qtile(vp.z(), tx, ty);
                  parallel++;
              }
              ret = false;
          }
       }
    }

    return ret;
}

