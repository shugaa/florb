#include <curl/curl.h>
#include <curl/easy.h>
#include <boost/bind.hpp>
#include <FL/Fl.H>

#include "downloader.hpp"

downloader::downloader(int nthreads) : 
    m_threadblock(0),
    m_exit(false)
{
    //nthreads=1;
    for (int i=0;i<nthreads;i++)
    {
        workerinfo *ti = new workerinfo(
            new boost::thread(boost::bind(&downloader::worker, this)));

#if 0
        struct sched_param param;
        param.sched_priority = 99;
        pthread_setschedparam(ti->t()->native_handle(), SCHED_RR, &param);
#endif

        m_workers.push_back(ti);
    }
}

downloader::~downloader()
{
    exit(true);

    // Post once for each worker thread
    for (size_t i=0;i<m_workers.size();i++)
    {
        m_threadblock.post();
    }

    // Join all active threads and delete them
    std::vector<workerinfo*>::iterator it;
    for (it=m_workers.begin();it!=m_workers.end();++it)
    {
        (*it)->t()->join();
        delete (*it)->t();
        delete (*it);
    }
}

bool downloader::queue(const std::string& url, void* userdata)
{   
    if (exit())
        return false;

    m_mutex.lock();

    // Find an existing item in the list
    bool newitem = false;
    download_internal d(this, url, userdata);
    std::vector<download_internal>::iterator it;
    for (it=m_queue.begin(); it!=m_queue.end(); ++it)
    {
        if ((*it) == d)
            break;
    }
    
    // Existing item, boost priority
    if (it != m_queue.end())
    {
        download_internal dtmp = (*it);
        m_queue.erase(it);
        m_queue.push_back(dtmp);
    }
    // New item, add to queue with max. priority
    else
    {
        m_queue.push_back(d);
        newitem = true;
    }

    m_mutex.unlock();

    // One more item on the list, post the counter semaphore
    if (newitem == true) 
    {
        m_threadblock.post();
    }

    return newitem;
}

bool downloader::get(download& dl)
{
    bool ret = false;

    m_mutex.lock();

    for (;;)
    {
        if (m_done.size() == 0)
            break;

        dl = *(m_done.end()-1);
        m_done.erase(m_done.end()-1);
        ret = true;

        break;
    }

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

void downloader::worker()
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

        //boost::this_thread::sleep_for(boost::chrono::milliseconds(200));

        // Get a download item from the list
        m_mutex.lock();
        download_internal dl = *(m_queue.end()-1);
        m_queue.erase(m_queue.end()-1);
        m_mutex.unlock();

        // Clear the buffer, reset the expires header
        dl.buf().resize(0);
        
        int rc;
        curl_easy_setopt(curl_handle, CURLOPT_URL, dl.url().c_str());
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &dl);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, &dl);

        // Start download
        rc = curl_easy_perform(curl_handle);

        // Download failed
        if (rc != 0)
            dl.buf().resize(0);

        m_mutex.lock();
        m_done.push_back(dl);
        m_mutex.unlock();

        // Fire event
        event_complete ce(this);
        fire(&ce);
    }

    curl_easy_cleanup(curl_handle);
}

size_t downloader::cb_data(void *ptr, size_t size, size_t nmemb, void *data)
{
    download_internal *d = reinterpret_cast<download_internal*>(data);
    return d->dldr()->handle_data(ptr, size, nmemb, d->buf());
}

size_t downloader::cb_header(void *ptr, size_t size, size_t nmemb, void *data) 
{
    download_internal *d = reinterpret_cast<download_internal*>(data);
    return d->dldr()->handle_header(ptr, size, nmemb, d->expires());
}

size_t downloader::handle_data(void *ptr, size_t size, size_t nmemb, std::vector<char>& buf)
{
    size_t realsize = size * nmemb;
    size_t currentsize = buf.size();

    buf.resize(currentsize + realsize);
    memcpy(reinterpret_cast<void*>(&(buf[currentsize])), ptr, realsize);

    return realsize;
}

size_t downloader::handle_header(void *ptr, size_t size, size_t nmemb, time_t& expires) 
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
    expires = curl_getdate(line.c_str(), NULL);

    return realsize;
}

