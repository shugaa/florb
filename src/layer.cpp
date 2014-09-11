#include <string>
#include <FL/Fl.H>
#include "layer.hpp"

florb::layer::layer() :
    m_name("N/A"),
    m_enabled(true)
{
    add_instance(this);
};

florb::layer::~layer()
{
    del_instance(this);
};

const std::string& florb::layer::name() const
{
    return m_name;
};

bool florb::layer::enabled() const
{
    return m_enabled;
};

void florb::layer::name(const std::string &name)
{
    m_name = name;
};

void florb::layer::enable(bool en)
{
    m_enabled = en;
};
