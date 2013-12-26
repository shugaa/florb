#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include <string>
#include <vector>
#include <set>
#include <boost/thread.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include "event.hpp"

class downloader : public event_generator
{
    public:
        downloader(int nthreads);
        ~downloader();

        bool queue(const std::string& url, void* userdata);

        class event_complete;

        class download
        {
            public:
                download() :
                    m_userdata(NULL) {};
                download(const std::string& url, void *userdata) :
                    m_expires(0),
                    m_url(url),
                    m_userdata(userdata) {};
                virtual ~download() {};

                const std::string& url() const { return m_url; };
                void *userdata() const { return m_userdata; };
                const std::vector<char>& buf() const { return m_buf; };
                const time_t& expires() const { return m_expires; };

                bool operator==(const download& d) const
                {
                    return (m_url == d.m_url);
                };
                bool operator<(const download& d) const
                {
                    return (m_url < d.m_url);
                };
                bool operator>(const download& d) const
                {
                    return (m_url > d.m_url);
                };

            protected:
                std::vector<char> m_buf;
                time_t m_expires;
                
            private:
                std::string m_url;
                void* m_userdata;
        };       

        bool get(download& dl);

    private:
        class download_internal : public download
        {
            public:
                download_internal(downloader* dldr, const std::string& url, void *userdata) :
                    download(url, userdata),
                    m_dldr(dldr) {};
                ~download_internal() {};

                downloader* dldr() const { return m_dldr; };
                std::vector<char>& buf() { return m_buf; };
                time_t& expires() { return m_expires; };

            private:
                downloader* m_dldr;
        };
        class workerinfo
        {   
            public:
                workerinfo(boost::thread* t) :
                    m_t(t) {};

                boost::thread* t() const { return m_t; };

            private:
                boost::thread *m_t;
        };

        void worker();

        static size_t cb_data(void *ptr, size_t size, size_t nmemb, void *data);
        static size_t cb_header(void *ptr, size_t size, size_t nmemb, void *data);
        size_t handle_data(void *ptr, size_t size, size_t nmemb, std::vector<char>& buf);
        size_t handle_header(void *ptr, size_t size, size_t nmemb, time_t& expires);

        bool exit(void);
        void exit(bool i);

        std::vector<download_internal> m_queue;
        std::vector<download_internal> m_done;

        std::vector<workerinfo*> m_workers;

        boost::interprocess::interprocess_semaphore m_threadblock;
        boost::interprocess::interprocess_mutex m_mutex;

        bool m_exit;
};

class downloader::event_complete : public event_base
{
    public:
        event_complete(downloader *dldr) :
            m_dldr(dldr) {};
        ~event_complete() {};
    
    private:
        downloader* m_dldr;
};

#endif // DOWNLOADER_HPP

