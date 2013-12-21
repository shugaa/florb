#ifndef EVENT_HPP
#define EVENT_HPP

#include <map>
#include <set>
#include <iostream>
#include <typeinfo>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

// Event base class
class event_base
{
    public:
        event_base() {};
        virtual ~event_base() {};
    private:
};

// type_info wrapper
class tinfo
{
    public:
        tinfo(const std::type_info* info) :
            m_info(info) {};

        bool operator==(tinfo const& i) const
        {
            return (m_info == i.m_info);
        };
        bool operator<(tinfo const& i) const
        {
            return (m_info < i.m_info);
        };
        bool operator>(tinfo const& i) const
        {
            return (m_info > i.m_info);
        };

    private:
        const std::type_info *m_info;
};

// Event handler base class
class event_handler
{
    public:
        virtual ~event_handler() {};
        bool exec(const event_base* evt) { return call(evt); };

    private:
        virtual bool call(const event_base*) = 0;
};

// An arbitrary object's member function handling an event
template <class T, class E>
class event_handler_memberfct : public event_handler
{
    public:
        // Note to self: f is a pointer to a member function of class T,
        // taking a "pointer to instance of E" parameter and returning void
        typedef bool (T::*f)(const E*);

        event_handler_memberfct(T* obj, f fn) :
            m_obj(obj),
            m_fn(fn) {};

        bool call(const event_base* evt)
        {
            // Static casting is safe here
            return (m_obj->*m_fn)(static_cast<const E*>(evt));
        };

    private:
        T* m_obj;
        f m_fn;
};

class event_listener
{
    public:
        event_listener() {};
        virtual ~event_listener(); 
        bool handle(const event_base* evt);
        bool handle_safe(const event_base* evt);

    private:
        class exec_info; 

        static void mt_callback(void *data);
        typedef std::map<tinfo, event_handler*> evthandlers;
        evthandlers m_evthandlers;

    protected:
        template<class T, class E>
        void register_event_handler(T* obj, bool (T::*f)(const E*))
        {
            m_evthandlers[tinfo(&typeid(E))] = new event_handler_memberfct<T, E>(obj, f);
        };
};

class event_generator
{
    public:
        event_generator() {};
        virtual ~event_generator() {};

        void add_event_listener(event_listener *l);
        void remove_event_listener(event_listener *l);

    private:
        std::set<event_listener*> m_listeners;

    protected:
        void fire(event_base* evt);
        void fire_safe(event_base* evt);
};

#endif // EVENT_HPP

