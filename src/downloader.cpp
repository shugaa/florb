#include <curl/curl.h>
#include <curl/easy.h>
#include <boost/bind.hpp>

#include "downloader.hpp"

downloader::downloader(void *userdata) : 
    m_threadblock(0),
    m_idle(true),
    m_exit(false),
    m_userdata(userdata)
{
    m_thread = new boost::thread(boost::bind(&downloader::worker, this));
}

downloader::~downloader()
{
    exit(true);
    m_threadblock.post();

    if (m_thread) 
    {
        m_thread->join();
        delete m_thread;
    }
}

void downloader::idle(bool i)
{
    m_mutex.lock();
    m_idle = i;
    m_mutex.unlock();
}

bool downloader::idle(void)
{
    bool ret;

    m_mutex.lock();
    ret = m_idle;
    m_mutex.unlock();

    return ret;
}

void downloader::exit(bool i)
{
    m_mutex.lock();
    m_exit = i;
    m_mutex.unlock();
}

bool downloader::exit(void)
{
    bool ret;

    m_mutex.lock();
    ret = m_exit;
    m_mutex.unlock();

    return ret;
}

void downloader::fetch(const std::string& url)
{
    if (!idle())
        return;

    idle(false);
    m_url = url;
    m_threadblock.post();
}

void* downloader::userdata(void)
{
    return m_userdata;
}

void downloader::worker(void)
{
    CURL *curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, this); 
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "florb/0.1");

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, cb_data);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, cb_header);

    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_handle, CURLOPT_DNS_USE_GLOBAL_CACHE, 0);

    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 15);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 10); 

    for (;;)
    {
        m_threadblock.wait();

        if (exit())
            break;

        // Clear the buffer, reset the expires header
        m_buf.resize(0);
        m_expires = 0;

        curl_easy_setopt(curl_handle, CURLOPT_URL, m_url.c_str());

        // Start download
        int rc = curl_easy_perform(curl_handle);

        // Download failed
        if (rc != 0)
            m_buf.resize(0);

        // Fire event
        event_complete ce(this, m_userdata, m_buf, m_expires);
        std::cout << "calling back" << std::endl;
        fire_safe(&ce);

        // Back to idle state
        idle(true);
    }

    curl_easy_cleanup(curl_handle);
}

size_t downloader::cb_data(void *ptr, size_t size, size_t nmemb, void *data)
{
    downloader *d = reinterpret_cast<downloader*>(data);
    return d->handle_data(ptr, size, nmemb);
}

size_t downloader::cb_header(void *ptr, size_t size, size_t nmemb, void *data) 
{
    downloader *d = reinterpret_cast<downloader*>(data);
    return d->handle_header(ptr, size, nmemb);
}

size_t downloader::handle_data(void *ptr, size_t size, size_t nmemb)
{
    size_t realsize = size * nmemb;
    size_t currentsize = m_buf.size();

    m_buf.resize(currentsize + realsize);
    memcpy(reinterpret_cast<void*>(&m_buf[currentsize]), ptr, realsize);

    return realsize;
}

size_t downloader::handle_header(void *ptr, size_t size, size_t nmemb) 
{
    size_t realsize = size * nmemb;
    std::string line;

    // Construct the whole line
    for (size_t i=0;i<realsize;i++)
        line += reinterpret_cast<char*>(ptr)[i];
    
    // Not an expires header
    if (line.find("Expires: ") == std::string::npos)
        return realsize;

    // Extract the date string from the expires header
    line = line.substr(strlen("Expires: "), line.size()-strlen("Expires: ")-1);    

    // Convert string to time_t
    m_expires = curl_getdate(line.c_str(), NULL);

    return realsize;
}

