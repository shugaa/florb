#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include <string>
#include <vector>
#include <boost/thread.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include "event.hpp"

class downloader : public event_generator
{
    public:
        downloader(void *userdata);
        ~downloader();

        bool idle(void);
        void* userdata(void);
        void fetch(const std::string& url);

        class event_complete;

    private:
        static size_t cb_data(void *ptr, size_t size, size_t nmemb, void *data);
        static size_t cb_header(void *ptr, size_t size, size_t nmemb, void *data);
        size_t handle_data(void *ptr, size_t size, size_t nmemb);
        size_t handle_header(void *ptr, size_t size, size_t nmemb);
        
        void idle(bool i);
        bool exit(void);
        void exit(bool i);

        void worker(void);
        
        std::string m_url;
        time_t m_expires;
        boost::thread *m_thread;
        std::vector<char> m_buf;
        boost::interprocess::interprocess_semaphore m_threadblock;
        boost::interprocess::interprocess_mutex m_mutex;
        bool m_idle;
        bool m_exit;
        void *m_userdata;
};

class downloader::event_complete : public event_base
{
    public:
        event_complete(downloader *dl, void* userdata, std::vector<char>& buf, time_t expires) :
            m_downloader(dl),
            m_userdata(userdata),
            m_buf(buf),
            m_expires(expires) {};
            ~event_complete() {};
    
        downloader* dl(void) const { return m_downloader; };
        time_t expires(void) const { return m_expires; }
        void* userdata(void) const  { return m_userdata; };
        std::vector<char>& buf(void) const { return m_buf; };
                
    private:
        downloader* m_downloader;
        void* m_userdata;
        std::vector<char>& m_buf;
        time_t m_expires;                
};

#endif // DOWNLOADER_HPP

