#include <cmath>
#include <iostream>
#include <vector>
#include "settings.hpp"
#include "viewport.hpp"
#include "utils.hpp"

florb::viewport::viewport(unsigned long w, unsigned long h)
{
    // See which zoomlevel fits best for the size request
    m_dim = 0;
    for (m_z=florb::viewport::ZMIN;m_z<=florb::viewport::ZMAX;m_z++)
    {
        m_dim = florb::utils::dim(m_z);

        // At this zoomlevel the map fills the viewport in both directions
        if ((m_dim >= w) && (m_dim >= h))
           break;
    }

    // Center the viewport over the map
    m_w = (m_dim <= w) ? m_dim : w;
    m_h = (m_dim <= h) ? m_dim : h;
    m_x = (m_dim <= m_w) ? 0 : (m_dim-m_w)/2;
    m_y = (m_dim <= m_h) ? 0 : (m_dim-m_h)/2;
};

florb::viewport::viewport(unsigned long x, unsigned long y, unsigned int z, unsigned long w, unsigned long h) :
    m_x(x), 
    m_y(y), 
    m_z(z), 
    m_w(w), 
    m_h(h),
    m_dim(florb::utils::dim(z))
{
};

florb::viewport::viewport() :
    m_x(0), 
    m_y(0), 
    m_z(0), 
    m_w(0), 
    m_h(0),
    m_dim(florb::utils::dim(0))
{
};

florb::viewport::~viewport()
{
};

void florb::viewport::intersect(const florb::viewport& vp)
{
   // Zoom level different, there can be no intersection
   if (z() != vp.z()) {
      m_x = 0; m_y = 0; m_w = 0; m_h = 0; m_z = vp.z();
      return;
   }

   unsigned long ax1 = m_x; 
   unsigned long ay1 = m_y; 
   unsigned long ax2 = m_x + m_w;
   unsigned long ay2 = m_y + m_h;
   unsigned long bx1 = vp.x(); 
   unsigned long by1 = vp.y(); 
   unsigned long bx2 = vp.x() + vp.w();
   unsigned long by2 = vp.y() + vp.h();

   // The viewport areas do not overlap
   if ((bx1 > ax2) ||
       (bx2 < ax1) ||
       (by1 > ay2) ||
       (by2 < ay1))
   {
      m_x = 0; m_y = 0; m_w = 0; m_h = 0;
      return;
   }

   // Areas do overlap, calculate and set the intersection
   m_x = ((ax1 > bx1) ? ax1 : bx1);
   m_y = ((ay1 > by1) ? ay1 : by1);
   m_w = ((ax2 < bx2) ? (ax2 - m_x) : (bx2 - m_x));
   m_h = ((ay2 < by2) ? (ay2 - m_y) : (by2 - m_y));
}

void florb::viewport::move(long dx, long dy)
{
    // Move in x direction
    if (dx >= 0)
       m_x += (unsigned long)dx;
    else if ((unsigned long)(-dx) > m_x)
       m_x = 0;
    else
       m_x -= (unsigned long)(-dx);

    // Move in y direction
    if (dy >= 0)
       m_y += (unsigned long)dy;
    else if ((unsigned long)(-dy) > m_y)
       m_y = 0;
    else
       m_y -= (unsigned long)(-dy);

    // Catch movements beyond the map
    if ((m_x + m_w) > m_dim)
       m_x = m_dim - m_w;
    
    if ((m_y + m_h) > m_dim)
       m_y = m_dim - m_h;
};

void florb::viewport::x(unsigned long x)
{
   m_x = x;
   
   // Catch movement beyond the map
   if ((m_x + m_w) > m_dim)
       m_x = m_dim - m_w;
}

void florb::viewport::y(unsigned long y)
{
   m_y = y;

   // Catch movement beyond the map
   if ((m_y + m_h) > m_dim)
       m_y = m_dim - m_h;
}

void florb::viewport::w(unsigned long w)
{
   // Viewport can not be larger than the map
   if (w > m_dim)
      w = m_dim;

   // Move the viewport to keep it centered
   unsigned long delta;
   if (w > m_w) {
      delta = (w-m_w)/2;
      m_x = (delta >= m_x) ? 0 : m_x-delta;
   } else {
      delta = (m_w-w)/2;
      m_x += delta;
   }

   m_w = w;

   if ((m_x + m_w) > m_dim)
      m_x = m_dim - m_w;
}

void florb::viewport::h(unsigned long h)
{
   // Viewport can not be larger than the map
   if (h > m_dim)
      h = m_dim;

   // Move the viewport to keep it centered
   unsigned long delta;
   if (h > m_h) {
      delta = (h-m_h)/2;
      m_y = (delta >= m_y) ? 0 : m_y-delta;
   } else {
      delta = (m_h-h)/2;
      m_y += delta;
   }

   m_h = h;

   if ((m_y + m_h) > m_dim)
      m_y = m_dim - m_h;
}

void florb::viewport::z(unsigned int z, unsigned long x, unsigned long y)
{
   // Invalid zoom values
   if (z > florb::viewport::ZMAX)
      return;
   if (z < florb::viewport::ZMIN)
      return;

   // Coordinates outside viewport
   if (x >= m_w)
      return;
   if (y >= m_h)
      return;

   // Calculate x and y factors to position the viewport over the map at the
   // new zoomlevel
   double facx = (double)(m_x+x)/(double)m_dim;
   double facy = (double)(m_y+y)/(double)m_dim;

   // Calculate the mapsize for the new zoomlevel and fit the viewport if
   // necessary
   m_z = z;
   m_dim = florb::utils::dim(m_z); 
   
   unsigned long newx = (unsigned long)((double)m_dim*facx);
   unsigned long newy = (unsigned long)((double)m_dim*facy);

   m_x = (newx > x) ? newx - x : 0;
   m_y = (newy > y) ? newy - y : 0;

   if (m_w > m_dim)
      m_w = m_dim;
   if (m_h > m_dim)
      m_h = m_dim;

   // Make sure the viewport didn't end up outside the map
   if ((m_x + m_w) > m_dim)
      m_x = m_dim - m_w;
   if ((m_y + m_h) > m_dim)
      m_y = m_dim - m_h;
}

bool florb::viewport::cmp(const florb::viewport& vp) 
{
    if (vp.m_x != m_x)
        return false;
    if (vp.m_y != m_y)
        return false;
    if (vp.m_z != m_z)
        return false;
    if (vp.m_w != m_w)
        return false;
    if (vp.m_h != m_h)
        return false;

    return true;
}
