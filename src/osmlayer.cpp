#include <cmath>
#include "utils.hpp"
#include "osmlayer.hpp"

#define ONE_WEEK                (7*24*60*60)
#define MAX_TILE_BACKLOG        (150)
#define TILE_W                  (256)
#define TILE_H                  (256)

osmlayer::osmlayer(std::string url, int numdownloads) :
    layer(),                        // Base class constructor 
    m_canvas_0(500, 500),           // Canvas for 'double buffering'. Will be resized as needed
    m_canvas_1(500, 500),           // Canvas for 'double buffering'. Will be resized as needed
    m_canvas_tmp(500,500),          // Temporary drawing canvas. Will be resized as needed
    m_shutdown(false),              // Shutdown flag
    m_url(url),                     // Tileserver URL
    m_numdownloads(numdownloads),   // Number of simultaneous downloads
    m_s(settings::get_instance())   // Settings instance reference
{
    // Set map name
    name("OSM Basemap");

    // Create cache
    m_cache = new sqlitecache(m_s["cache"]["location"].as<std::string>());
    m_cache->sessionid(url);
};

osmlayer::~osmlayer()
{
    // No new downloads will be started by asynchronous callbacks
    m_shutdown = true;

    // Clear the download queue, only active downloads may finish now
    m_downloadq.clear();

    // Wait for all pending downloads to be processed. download_process() will
    // usually only be called from the UI thread through FL::awake.
    // Unfortunately the UI thread is now stuck right here, waiting for our
    // destruction. So we need to poll the status of each remaining download.
    do {
        // Wait for something to happen, specifically the callback routine
        // issued by the pending download
        Fl::wait();
    } while (m_downloads.size() > 0);

    // Destroy the cache
    delete m_cache;
};

void osmlayer::download_notify(void)
{
    Fl::awake(osmlayer::download_callback, (void*)this);
}

void osmlayer::download_callback(void *data)
{
    osmlayer *m = reinterpret_cast<osmlayer*>(data);
    m->download_process();
}

void osmlayer::download_process(void)
{
    // There was a download status change, check what has happened
    for(std::vector<dlref_t>::iterator it=m_downloads.begin(); it!=m_downloads.end(); ) 
    {
        switch ((*it).dl->status()) 
        {
            case download::FINISHED:
            {
                // Check the expires header
                time_t expires = (*it).dl->expires();
                time_t now = time(NULL);
                if (expires <= now)
                    expires = now + ONE_WEEK;

                // Cache the tile and ask for redraw
                m_cache->put((*it).t.z, (*it).t.x, (*it).t.y, expires, (*it).dl->buf());
                notifyobservers();   
            }
            // Fall through
            case download::ERROR:
                // Remove the download
                delete (*it).dl;
                it = m_downloads.erase(it);
                break;
            default:
                // In Progress
                ++it;
        }
    }
       
    // Maybe we can start another download
    if (!m_shutdown) 
        download_startnext();
}

void osmlayer::download_startnext(void)
{
    // No more free download slots
    if (m_downloads.size() >= m_numdownloads)
        return;

    // See whether there are more requests in the queue
    if (m_downloadq.size() == 0)
        return;

    // Get the request from the queue and start it...
    tile_t next = m_downloadq.back();
    m_downloadq.pop_back();


    //...if it is not already in the cache
    if (m_cache->exists(next.z, next.x, next.y) == sqlitecache::FOUND)
        return;

    // Consruct the url
    std::ostringstream sz, sx, sy;
    sz << next.z;
    sx << next.x;
    sy << next.y;

    std::string url(m_url);
    url.replace (url.find("$FLORBZ$"), std::string("$FLORBZ$").length(), sz.str());
    url.replace (url.find("$FLORBX$"), std::string("$FLORBX$").length(), sx.str());
    url.replace (url.find("$FLORBY$"), std::string("$FLORBY$").length(), sy.str());

    // Create and start the download
    download *dl = new download(url, this);

    // Put the download into the queue
    dlref_t ndl;
    ndl.dl = dl;
    ndl.t = next;
    m_downloads.push_back(ndl);

    // Start the download
    dl->start();
}

void osmlayer::download_qtile(const tile_t &tile)
{
    // Queue is full, erase oldest entry
    if (m_downloadq.size() >= MAX_TILE_BACKLOG)
        m_downloadq.erase(m_downloadq.begin());

    // Add tile request to queue
    m_downloadq.push_back(tile);

    // Eventually start the next download
    download_startnext();
}

void osmlayer::draw(const viewport &viewport, canvas &os)
{
    update_map(viewport);
    os.draw(m_canvas_0, 0, 0, (int)viewport.w(), (int)viewport.h(), 0, 0);
}

void osmlayer::update_map(const viewport &vp)
{
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
          int rc = m_cache->get(vp.z(), tx, ty, m_imgbuf);
          
          // Draw the tile if we either have a valid or expired version of it...
          if (rc != sqlitecache::NOTFOUND)
          {
              image img(image::PNG, (unsigned char*)(&m_imgbuf[0]), m_imgbuf.size());
              c.draw(img, px, py);
          }
          // ...otherwise draw placeholder image
          else
          {
              c.fillrect(px, py, TILE_W, TILE_H, 177, 113, 113);
          }

          // Tile not in cache or expired
          if ((rc == sqlitecache::EXPIRED) || 
              (rc == sqlitecache::NOTFOUND))
          {
              tile_t t;
              t.z = vp.z();
              t.x = tx; 
              t.y = ty;
              download_qtile(t);
              ret = false;
          }
       }
    }

    return ret;
}

