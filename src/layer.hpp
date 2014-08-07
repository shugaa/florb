#ifndef LAYER_HPP
#define LAYER_HPP

#include <set>
#include <string>
#include "gfx.hpp"
#include "viewport.hpp"
#include "point.hpp"
#include "event.hpp"

class layer : public event_listener, public event_generator
{
    public:
        layer();
        virtual ~layer();

        virtual bool draw(const viewport &viewport, fgfx::canvas &c) = 0;
        const std::string& name() const;

        static bool is_instance(layer* l);
        void enable(bool en);

        // Predefined layer events
        class event_mouse;
        class event_key;
    private:
        static std::set<layer*> m_instances;
        std::string m_name;
        bool m_enabled;
        
    protected:
        bool enabled() const;
        void name(const std::string &name);
};

class layer::event_mouse : public event_base
{
    public:
        event_mouse(const viewport& vp, int action, int button, point2d<int> pos) :
            event_base(),
            m_viewport(vp),
            m_action(action),
            m_button(button),
            m_pos(pos)
            {};
        ~event_mouse() {}; 

        const viewport& vp() const { return m_viewport; }; 
        int action() const { return m_action; };
        int button() const { return m_button; };
        point2d<int> pos() const { return m_pos; };

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
        point2d<int> m_pos;
};

class layer::event_key : public event_base
{
    public:
        event_key(int action, int key) :
            event_base(),
            m_action(action),
            m_key(key)
            {};
        ~event_key() {}; 

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

