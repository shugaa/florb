#include "event.hpp"

event_listener::~event_listener()
{
    evthandlers::iterator it;
    for (it=m_evthandlers.begin();it!=m_evthandlers.end();++it)
    {
        std::cout << "destroy handler" << std::endl;
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

