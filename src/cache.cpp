#include <sstream>
#include <cstring>
#include <iostream>
#include <fstream>
#include "settings.hpp"
#include "cache.hpp"
#include "utils.hpp"

const std::string cache::dbextension = ".dat";

cache::cache(const std::string& url, const std::string& session, const std::string& ext) :
    m_url(url),
    m_session(session),
    m_ext(ext)
{
    int rc = 0;

    for (;;)
    {
        if (!utils::exists(m_url))
        {
            rc = -1;
            break;
        }

        if (utils::exists(m_url+utils::pathsep()+m_session))
        {
            break;
        }

        try {
            utils::mkdir(m_url+utils::pathsep()+m_session);
        } catch (...) {
            rc = -1;
            break;
        }

        break;
    }

    // An error occured
    if (rc != 0) 
    {
        throw std::runtime_error(_("Failed to open / create cache database"));;
    }
};

cache::~cache()
{
};

void cache::put(int z, int x, int y, time_t expires, const std::vector<char> &buf)
{
    if ((z < 0) || (x < 0) || (y < 0))
        return;

    int rc = 0;
    for (;;) 
    {
        // Create the storage directory for the tile if not already present
        std::ostringstream oss;
        
        oss << m_url << utils::pathsep() << m_session; 
        if (!utils::exists(oss.str()))
            utils::mkdir(oss.str());

        oss << utils::pathsep() << z;
        if (!utils::exists(oss.str()))
            utils::mkdir(oss.str());

        oss << utils::pathsep() << x;
        if (!utils::exists(oss.str()))
            utils::mkdir(oss.str());

        oss << utils::pathsep() << y << m_ext;

        std::ofstream of;
        of.open(oss.str().c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!of.is_open())
        {
            rc = -1;
            break;
        }

        of.write(&(buf[0]), buf.size());
        of.close();

        oss << dbextension;

        of.open(oss.str().c_str(), std::ios::out | std::ios::trunc);
        if (!of.is_open())
        {
            rc = -1;
            break;
        }

        of << expires;
        of.close();
        
        break;
    }

    if (rc != 0)
    {
        throw std::runtime_error(_("Cache error: PUT"));
    }
}

int cache::exists(int z, int x, int y)
{
    if ((z < 0) || (x < 0) || (y < 0))
        return false;

    int rc = -1;
    for (;;)
    {
        std::string sep(utils::pathsep());
        std::ostringstream oss;
        oss << m_url << sep << m_session << sep << z << sep << x << sep << y << m_ext; 

        std::ifstream tf;
        tf.open(oss.str().c_str(), std::ios::in);

        if (!tf.is_open())
        {
            rc = NOTFOUND;
            break;
        }
        tf.close();

        rc = FOUND;

        oss << dbextension;
        tf.open(oss.str().c_str(), std::ios::in);
        if (!tf.is_open())
        {
            rc = EXPIRED;
            break;
        }

        time_t expires;
        tf >> expires;
        tf.close();

        time_t now = time(NULL);
        if (now > expires)
            rc = EXPIRED;

        break;
    }

    if (rc < 0)
    {
        throw std::runtime_error(_("Cache error: EXISTS"));
    }

    return rc;
}

int cache::get(int z, int x, int y, std::vector<char> &buf)
{
    if ((z < 0) || (x < 0) || (y < 0))
        return NOTFOUND;

    int rc = -1;
    for (;;)
    {
        // Return the data
        std::ostringstream oss;
        std::string sep(utils::pathsep());
        oss << m_url << sep << m_session << sep << z << sep << x << sep << y << m_ext; 

        std::ifstream tf;
        tf.open(oss.str().c_str(), std::ios::in | std::ios::binary);

        if (!tf.is_open())
        {
            rc = NOTFOUND;
            break;
        }

        tf.seekg(0, tf.end);
        size_t msize = tf.tellg();
        tf.seekg(0, tf.beg);
        
        buf.resize(msize);
        tf.read(&(buf[0]), msize);
        tf.close();

        time_t expires;
        oss << dbextension;
        tf.open(oss.str().c_str(), std::ios::in | std::ios::binary);

        if (!tf.is_open())
        {
            rc = EXPIRED;
            break;
        }

        rc = FOUND;

        tf >> expires;
        tf.close();

        // Check whether this tile has expired
        time_t now = time(NULL);
        if (now > expires)
            rc = EXPIRED;

        break;
    }

    if (rc < 0)
    {
        throw std::runtime_error(_("Cache error: GET"));
    }

    return rc;
}

