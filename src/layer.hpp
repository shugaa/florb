#ifndef LAYER_HPP
#define LAYER_HPP

#include <set>
#include <string>
#include "gfx.hpp"
#include "viewport.hpp"

// Forward declare observer class
class layer_observer;

class layer
{
    public:
        layer();
        virtual ~layer();

        virtual void draw(const viewport &viewport, canvas &c) = 0;
        const std::string& name();

        void addobserver(layer_observer &o);
        void removeobserver(layer_observer &o);

        void notifyobservers();
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

#endif // LAYER_HPP

