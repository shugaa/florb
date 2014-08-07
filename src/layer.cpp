#include <string>
#include <FL/Fl.H>
#include "layer.hpp"

std::set<layer*> layer::m_instances;

layer::layer() :
    m_name("N/A"),
    m_enabled(true)
{
    m_instances.insert(this);
};

layer::~layer()
{
    // Not a valid layer instance anymore
    m_instances.erase(m_instances.find(this));
};

const std::string& layer::name() const
{
    return m_name;
};

bool layer::enabled() const
{
    return m_enabled;
};

void layer::name(const std::string &name)
{
    m_name = name;
};

void layer::enable(bool en)
{
    m_enabled = en;
};

bool layer::is_instance(layer* l)
{
    std::set<layer*>::iterator it = m_instances.find(l);
    if (it == m_instances.end())
        return false;

    return true;
}
