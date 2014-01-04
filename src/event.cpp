#include <FL/Fl.H>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include "event.hpp"

class event_listener::exec_info 
{
    public:
        exec_info(event_handler* h, const event_base* e) :
            m_mutex(0), m_h(h), m_e(e) { };
        ~exec_info() { };
        event_handler* handler() { return m_h; };
        const event_base* event() { return m_e; };
        void wait() { m_mutex.wait(); };
        void unlock() { m_mutex.post(); };
        bool ret() { return m_ret; };
        void ret (bool rc) { m_ret = rc; };
    private:
        boost::interprocess::interprocess_semaphore m_mutex;
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
    {
        return false;
    }

    exec_info execinfo(it->second, evt);
    if (Fl::awake(event_listener::mt_callback, (void*)&execinfo) != 0)
    {
        return false;
    }

    execinfo.wait();

    return execinfo.ret();
}

void event_listener::mt_callback(void *data)
{
    exec_info *execinfo = static_cast<exec_info*>(data);
    execinfo->ret((execinfo->handler())->exec(execinfo->event()));
    execinfo->unlock();
}

void event_generator::add_event_listener(event_listener *l)
{
    m_listeners.insert(l);
}

void event_generator::remove_event_listener(event_listener *l)
{
    std::set<event_listener*>::iterator it = m_listeners.find(l);
    if (it != m_listeners.end())
        m_listeners.erase(it);
}

bool event_generator::fire(const event_base* evt)
{
    bool ret = false;
    
    std::set<event_listener*>::iterator it;
    for (it = m_listeners.begin(); it != m_listeners.end();++it)
    {
        if((*it)->handle(evt))
            ret = true;
    }

    return ret;
}

bool event_generator::fire_safe(const event_base* evt)
{
    bool ret = false;

    std::set<event_listener*>::iterator it;
    for (it = m_listeners.begin(); it != m_listeners.end();++it)
    {
        if ((*it)->handle_safe(evt))
            ret = true;
    }

    return ret;
}

