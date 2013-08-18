#ifndef DOWNLOAD_HPP
#define DOWNLOAD_HPP

#include <string>
#include <vector>
#include <boost/thread.hpp>

// Forward declare observer class
class download_observer;

class download
{
    public:
        download(const std::string url, download_observer *observer);
        ~download();

        void start(void);
        int status(void) { return m_status; };
        const std::vector<char> &buf(void) { return m_buf; };
        time_t expires(void) { return m_expires; };    

        enum {
            ERROR, 
            FINISHED,
            INPROGRESS,
        };

    private:
        static size_t cb_data(void *ptr, size_t size, size_t nmemb, void *data);
        static size_t cb_header(void *ptr, size_t size, size_t nmemb, void *data);
        size_t handle_data(void *ptr, size_t size, size_t nmemb);
        size_t handle_header(void *ptr, size_t size, size_t nmemb);
        
        void done(void);
        void worker(void);
        
        std::string m_url;
        time_t m_expires;
        boost::thread *m_thread;
        std::vector<char> m_buf;
        download_observer *m_observer;
        int m_status;
};

class download_observer
{
    public:
        virtual void download_notify(void) = 0;
};

#endif // DOWNLOAD_HPP
