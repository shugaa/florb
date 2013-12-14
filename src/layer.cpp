#include <string>
#include <FL/Fl.H>
#include "layer.hpp"

layer::layer() :
    m_name("N/A")
{
    ;
};

layer::~layer()
{
    ;
};

const std::string& layer::name()
{
    return m_name;
};

void layer::name(const std::string &name)
{
    m_name = name;
};

void layer::addobserver(layer_observer &o)
{
    m_observers.insert(&o);
};

void layer::removeobserver(layer_observer &o)
{
    m_observers.erase(&o);
};

void layer::notify_observers()
{
    std::set<layer_observer*>::iterator it;
    for (it = m_observers.begin(); it != m_observers.end(); it++)
        (*it)->layer_notify();
};

