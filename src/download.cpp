#include <curl/curl.h>
#include <curl/easy.h>
#include <boost/bind.hpp>

#include "download.hpp"

download::download(std::string url, download_observer *observer) : 
    m_url(url),
    m_observer(observer),
    m_status(INPROGRESS)
{
}

download::~download()
{
    if (m_thread) 
    {
        m_thread->join();
        delete m_thread;
    }
}

void download::start(void)
{
    m_thread = new boost::thread(boost::bind(&download::worker, this));
}

void download::done(void)
{
    // Notify observer
    if (m_observer)
        m_observer->download_notify();
}

void download::worker(void)
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

    curl_easy_setopt(curl_handle, CURLOPT_URL, m_url.c_str());

    // Start download
    int rc = curl_easy_perform(curl_handle);

    curl_easy_cleanup(curl_handle);

    // Download failed
    if (rc != 0)
        m_status = ERROR;
    else
        m_status = FINISHED;

    // Notify observer
    done();
}

size_t download::cb_data(void *ptr, size_t size, size_t nmemb, void *data)
{
    download *d = reinterpret_cast<download*>(data);
    return d->handle_data(ptr, size, nmemb);
}

size_t download::cb_header(void *ptr, size_t size, size_t nmemb, void *data) 
{
    download *d = reinterpret_cast<download*>(data);
    return d->handle_header(ptr, size, nmemb);
}

size_t download::handle_data(void *ptr, size_t size, size_t nmemb)
{
    size_t realsize = size * nmemb;
    size_t currentsize = m_buf.size();

    m_buf.resize(currentsize + realsize);
    memcpy(reinterpret_cast<void*>(&m_buf[currentsize]), ptr, realsize);

    return realsize;
}

size_t download::handle_header(void *ptr, size_t size, size_t nmemb) 
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
