#ifndef LAYER_HPP
#define LAYER_HPP

#include <set>
#include <string>
#include <map>
#include <iostream>
#include <typeinfo>
#include "gfx.hpp"
#include "viewport.hpp"
#include "point.hpp"
#include "event.hpp"

// Forward declare observer class
class layer_observer;

class layer : public event_listener
{
    public:
        layer();
        virtual ~layer();

        virtual void draw(const viewport &viewport, canvas &c) = 0;
        const std::string& name();

        void addobserver(layer_observer &o);
        void removeobserver(layer_observer &o);

        void notify_observers();
    private:
        std::string m_name;
        std::set<layer_observer*> m_observers;
        
    protected:
        void name(const std::string &name);
};

class layer_observer
{
    public:
        virtual void layer_notify() = 0;
};

class layer_mouseevent : public event_base
{
    public:
        layer_mouseevent(const viewport& vp, int action, int button, point2d<unsigned long> pos) :
            event_base(),
            m_viewport(vp),
            m_action(action),
            m_button(button),
            m_pos(pos)
            {};
        ~layer_mouseevent() {}; 

        const viewport& vp() const { return m_viewport; }; 
        int action() const { return m_action; };
        int button() const { return m_button; };
        point2d<unsigned long> pos() const { return m_pos; };

        enum {
            BUTTON_LEFT,
            BUTTON_MIDDLE,
            BUTTON_RIGHT
        };
        enum {
            ACTION_PRESS,
            ACTION_RELEASE,
            ACTION_DRAG
        };
        
    private:
        viewport m_viewport;
        int m_action;
        int m_button;
        point2d<unsigned long> m_pos;
};

class layer_keyevent : public event_base
{
    public:
        layer_keyevent(int action, int key) :
            event_base(),
            m_action(action),
            m_key(key)
            {};
        ~layer_keyevent() {}; 

        int action() const { return m_action; };
        int key() const { return m_key; };

        enum {
            KEY_DEL
        };
        enum {
            ACTION_PRESS,
            ACTION_RELEASE
        };
        
    private:
        int m_action;
        int m_key;
};

#endif // LAYER_HPP

