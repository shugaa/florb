#include <yaml-cpp/yaml.h>
#include <fstream>
#include "settings.hpp"

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
         return node;
      }

      static bool decode(const Node& node, cfg_tileserver& rhs) {
         if(!node.IsMap() || node.size() != 5)
            return false;

         rhs.name = node["name"].as<std::string>();
         rhs.url = node["url"].as<std::string>();
         rhs.zmin = node["zmin"].as<int>();
         rhs.zmax = node["zmax"].as<int>();
         rhs.parallel= node["parallel"].as<int>();
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

settings::settings() :
    m_rootnode(std::string("/home/bjoern/florb2.cfg"))
{
};

settings::~settings()
{
    m_rootnode.serialize(std::string("/home/bjoern/florb2.cfg"));
};

settings& settings::get_instance()
{
    static settings instance;
    return instance;
};

node::node(const std::string& path)
{
    m_ref = new yaml_node(path);
};

node::~node()
{
    delete m_ref;
};

node node::operator[] (const int idx)
{
    return node(new yaml_node(m_ref->get()[idx]));
};

node node::operator[] (const std::string &name)
{
    return node(new yaml_node(m_ref->get()[name]));
};

template<typename T> node& node::operator= (const T& rhs)
{
    m_ref->get() = rhs;
    return *this;
};

template<typename T> T node::as() const
{
    return m_ref->get().as<T>();
};

template<typename T> void node::push_back(const T& rhs)
{
    m_ref->get().push_back<T>(rhs);
};

void node::push_back(const node& rhs)
{
    m_ref->get().push_back(rhs.m_ref->get());
};

void node::serialize(const std::string& path)
{
    std::ofstream fout(path.c_str());
    fout << m_ref->get();
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
