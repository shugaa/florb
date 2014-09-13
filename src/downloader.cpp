#include <curl/curl.h>
#include <curl/easy.h>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <FL/Fl.H>

#include "utils.hpp"
#include "version.hpp"
#include "downloader.hpp"

florb::downloader::downloader(int nthreads) : 
    m_timeout(10),
    m_nice(0),
    m_stat(0),
    m_threadblock(0),
    m_exit(false)
{
    for (int i=0;i<nthreads;i++)
    {
        florb::downloader::workerinfo *ti;
        try {
            // Try to create a new worker thread
            ti = new florb::downloader::workerinfo(
                new boost::thread(boost::bind(&florb::downloader::worker, this)));
        } catch (...) {
            // Delete all previously created threads
            do_exit(true);
            for (int j=0;j<i;j++)
            {
                m_workers[j]->t()->join();
                delete m_workers[j];
            }

            throw std::runtime_error(_("Failed to start downloader"));
        }

        m_workers.push_back(ti);
    }
}

florb::downloader::~downloader()
{
    // Set exit flag
    do_exit(true);

    // Post once for each worker thread
    for (size_t i=0;i<m_workers.size();i++)
    {
        m_threadblock.post();
    }

    // Join all active threads and delete them
    std::vector<florb::downloader::workerinfo*>::iterator it;
    for (it=m_workers.begin();it!=m_workers.end();++it)
    {
        (*it)->t()->join();
        delete (*it);
    }
}

void florb::downloader::nice(long ms)
{
    m_mutex.lock();
    m_nice = ms;
    m_mutex.unlock();
}

long florb::downloader::nice()
{   
    long ret;

    m_mutex.lock();
    ret = m_nice;
    m_mutex.unlock();

    return ret;
}

std::size_t florb::downloader::stat()
{
    m_mutex.lock();
    std::size_t ret = m_stat;
    m_mutex.unlock();

    return ret;
}

void florb::downloader::stat(std::size_t s)
{
    m_mutex.lock();
    m_stat = s;
    m_mutex.unlock();
}

void florb::downloader::timeout(size_t sec)
{
    m_mutex.lock();
    m_timeout = sec;
    m_mutex.unlock();
}

size_t florb::downloader::timeout()
{
    size_t ret;

    m_mutex.lock();
    ret = m_timeout;
    m_mutex.unlock();

    return ret;
}

bool florb::downloader::queue(const std::string& url, void* userdata)
{   
    if (do_exit())
        return false;

    // Very basic URL encoding
    std::string urle(url);
    size_t pos;
    while((pos = urle.find(" ")) != std::string::npos)
        urle.replace(pos, 1, "%20"); 

    m_mutex.lock();

    // Find an existing item in the list
    bool newitem = false;
    florb::downloader::download_internal d(this, urle, userdata);
    std::vector<florb::downloader::download_internal>::iterator it;
    for (it=m_queue.begin(); it!=m_queue.end(); ++it)
    {
        if ((*it) == d)
            break;
    }
    
    // Existing item, boost priority
    if (it != m_queue.end())
    {
        florb::downloader::download_internal dtmp = (*it);
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
    if (newitem) 
    {
        m_threadblock.post();
    }

    return newitem;
}

size_t florb::downloader::qsize()
{
    size_t ret;
    m_mutex.lock();
    ret = m_queue.size();
    m_mutex.unlock();

    return ret;
}

bool florb::downloader::get(florb::downloader::download& dl)
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

void florb::downloader::do_exit(bool i)
{
    m_mutex.lock();
    m_exit = i;
    m_mutex.unlock();
}

bool florb::downloader::do_exit(void)
{
    bool ret;

    m_mutex.lock();
    ret = m_exit;
    m_mutex.unlock();

    return ret;
}

void florb::downloader::worker()
{
    CURL *curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, this); 
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, FLORB_USERAGENT);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, cb_data);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, cb_header);

    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_handle, CURLOPT_DNS_USE_GLOBAL_CACHE, 0);

    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout());
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, timeout()); 
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1); 

    for (;;)
    {
        // Wait for request
        m_threadblock.wait();

        // Exit?
        if (do_exit())
            break;

        // Get a download item from the list
        m_mutex.lock();
        florb::downloader::download_internal dl = *(m_queue.end()-1);
        m_queue.erase(m_queue.end()-1);
        m_mutex.unlock();

        // Clear the buffer
        dl.buf().resize(0);
        
        curl_easy_setopt(curl_handle, CURLOPT_URL, dl.url().c_str());
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &dl);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, &dl);

        // Start download
        if (curl_easy_perform(curl_handle) != 0)
        {
            // Download failed return an empty buffer
            dl.buf().resize(0);
        }

        // Check http status code
        long httprc = 0;
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &httprc);
        dl.httprc(httprc);

        // Finish download and update statistics
        m_mutex.lock();
        m_done.push_back(dl);
        m_stat++;
        m_mutex.unlock();

        // Fire event
        event_complete ce(this);
        fire(&ce);

        // Sleep
        if (nice() > 0)
            boost::this_thread::sleep(boost::posix_time::milliseconds(nice()));
    }

    curl_easy_cleanup(curl_handle);
}

size_t florb::downloader::cb_data(void *ptr, size_t size, size_t nmemb, void *data)
{
    florb::downloader::download_internal *d = 
        reinterpret_cast<florb::downloader::download_internal*>(data);
    
    return d->dldr()->handle_data(ptr, size, nmemb, d->buf());
}

size_t florb::downloader::cb_header(void *ptr, size_t size, size_t nmemb, void *data) 
{
    florb::downloader::download_internal *d = 
        reinterpret_cast<florb::downloader::download_internal*>(data);

    return d->dldr()->handle_header(ptr, size, nmemb, d->expires());
}

size_t florb::downloader::handle_data(void *ptr, size_t size, size_t nmemb, std::vector<char>& buf)
{
    size_t realsize = size * nmemb;
    size_t currentsize = buf.size();

    buf.resize(currentsize + realsize);
    memcpy(reinterpret_cast<void*>(&(buf[currentsize])), ptr, realsize);

    return realsize;
}

size_t florb::downloader::handle_header(void *ptr, size_t size, size_t nmemb, time_t& expires) 
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

