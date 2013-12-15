#include <yaml-cpp/yaml.h>
#include <fstream>
#include "settings.hpp"
#include "gfx.hpp"

namespace YAML {
   template<>
   struct convert<cfg_tileserver> {
      static Node encode(const cfg_tileserver& rhs) {
         Node node;
         node["name"] = rhs.name;
         node["url"] = rhs.url;
         node["zmin"] = rhs.zmin;
         node["zmax"] = rhs.zmax;
         node["parallel"] = rhs.parallel;

         node["type"] = "PNG";
         if      (rhs.type == image::PNG)
            node["type"] = "PNG";
         else if (rhs.type == image::JPG)
            node["type"] = "JPG";

         return node;
      }

      static bool decode(const Node& node, cfg_tileserver& rhs) {
         if(!node.IsMap() || node.size() != 6)
            return false;

         rhs.name = node["name"].as<std::string>();
         rhs.url = node["url"].as<std::string>();
         rhs.zmin = node["zmin"].as<int>();
         rhs.zmax = node["zmax"].as<int>();
         rhs.parallel= node["parallel"].as<int>();
         
         rhs.type = image::PNG;
         std::string imgtype = node["type"].as<std::string>();
         if      (imgtype.compare("PNG") == 0)
            rhs.type = image::PNG;
         else if (imgtype.compare("JPG") == 0)
            rhs.type = image::JPG;
         
         return true;
      }
   };
}

class yaml_node
{
    public:
        yaml_node(YAML::Node n) : m_node(n) {};
        yaml_node(const std::string& path)
        {
            m_node = YAML::LoadFile(path);
        }
        ~yaml_node() {};

        YAML::Node& get() { return m_node; };
    private:
        YAML::Node m_node;  
};

class yaml_iterator
{
    public:
        yaml_iterator(YAML::iterator i) : m_iter(i) {};
        ~yaml_iterator() {};

        YAML::iterator& get() { return m_iter; };
    private:
        YAML::iterator m_iter;  
};

node::iterator::iterator(const node& n, int be) 
{
    if (be <= 0)
        m_ref = new yaml_iterator(n.m_ref->get().begin());
    else
        m_ref = new yaml_iterator(n.m_ref->get().end());
}

node::iterator::~iterator()
{   
    delete m_ref;
}

bool node::iterator::operator==(iterator const& rhs) const 
{
    return (m_ref->get() == rhs.m_ref->get());
}

bool node::iterator::operator!=(iterator const& rhs) const 
{
    return !(m_ref->get() == rhs.m_ref->get());
}

node::iterator& node::iterator::operator++() 
{
    (m_ref->get())++;
    return *this;
}   

node::iterator node::iterator::operator++(int) 
{
    iterator tmp (*this);
    ++(*this);
    return tmp;
}

// Bidirectional iterators are not supported by YAML-Cpp
#if 0
iterator& node::iterator::operator--() 
{
    (m_ref.get())--;
    return *this;
}

iterator node::iterator::operator--(int) 
{
    iterator tmp (*this);
    --(*this);
    return tmp;
}
#endif

node node::iterator::operator* () const 
{
    yaml_node *tmp = new yaml_node(*(m_ref->get()));
    return node(tmp);
}

settings::settings() :
    m_rootnode(std::string("/home/bjoern/florb2.cfg"))
{
}

settings::~settings()
{
    m_rootnode.serialize(std::string("/home/bjoern/florb2.cfg"));
}

settings& settings::get_instance()
{
    static settings instance;
    return instance;
}

node::node(const std::string& path)
{
    m_ref = new yaml_node(path);
}

node::node(const node& n)
{
    m_ref = new yaml_node(n.m_ref->get());
}

node::~node()
{
    delete m_ref;
}

node node::operator[] (const int idx)
{
    return node(new yaml_node(m_ref->get()[idx]));
}

node node::operator[] (const std::string &name)
{
    return node(new yaml_node(m_ref->get()[name]));
}

template<typename T> node& node::operator= (const T& rhs)
{
    m_ref->get() = rhs;
    return *this;
}

template<typename T> T node::as() const
{
    return m_ref->get().as<T>();
}

template<typename T> void node::push_back(const T& rhs)
{
    m_ref->get().push_back<T>(rhs);
}

void node::push_back(const node& rhs)
{
    m_ref->get().push_back(rhs.m_ref->get());
}

void node::serialize(const std::string& path)
{
    std::ofstream fout(path.c_str());
    fout << m_ref->get();
}

size_t node::size()
{
    return m_ref->get().size();
}

// This is necessary so YAML::Node template internals remain hidden from the
// header file
template int node::as<int>() const;
template std::string node::as<std::string>() const;
template cfg_tileserver node::as<cfg_tileserver>() const;
template std::vector< cfg_tileserver > node::as< std::vector<cfg_tileserver> >() const;

template void node::push_back<int>(const int& rhs);
template void node::push_back<std::string>(const std::string& rhs);
template void node::push_back<cfg_tileserver>(const cfg_tileserver& rhs);

template node& node::operator=<int> (const int& rhs);
template node& node::operator=< std::vector<int> > (const std::vector<int>& rhs);
template node& node::operator=<std::string> (const std::string& rhs);
template node& node::operator=< std::vector<cfg_tileserver> > (const std::vector<cfg_tileserver>& rhs);
template node& node::operator=<cfg_tileserver> (const cfg_tileserver& rhs);
