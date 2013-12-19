#include <FL/Fl.H>
#include "event.hpp"

class event_listener::exec_info 
{
    public:
        exec_info(event_handler* h, const event_base* e) :
            m_h(h), m_e(e) { m_mutex.lock(); };
        ~exec_info() { m_mutex.unlock(); };
        event_handler* handler() { return m_h; };
        const event_base* event() { return m_e; };
        void wait() { m_mutex.lock(); };
        void unlock() { m_mutex.unlock(); };
        bool ret() { return m_ret; };
        void ret (bool rc) { m_ret = rc; };
    private:
        boost::interprocess::interprocess_mutex m_mutex;
        event_handler* m_h;
        const event_base* m_e;
        bool m_ret;
};

event_listener::~event_listener()
{
    evthandlers::iterator it;
    for (it=m_evthandlers.begin();it!=m_evthandlers.end();++it)
    {
        delete (*it).second;
    }
}

bool event_listener::handle(const event_base* evt)
{
    evthandlers::iterator it = m_evthandlers.find(tinfo(&typeid(*evt)));
    if(it != m_evthandlers.end())
        return (*it).second->exec(evt);

    return false;
}

bool event_listener::handle_safe(const event_base* evt)
{
    evthandlers::iterator it = m_evthandlers.find(tinfo(&typeid(*evt)));
    if(it == m_evthandlers.end())
        return false;

    exec_info execinfo(it->second, evt);
    Fl::awake(event_listener::mt_callback, (void*)&execinfo);
    execinfo.wait();

    return execinfo.ret();
}

void event_listener::mt_callback(void *data)
{
    exec_info *execinfo = static_cast<exec_info*>(data);
    execinfo->ret((execinfo->handler())->exec(execinfo->event()));
    execinfo->unlock();
};

