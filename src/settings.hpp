#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>

// The YAML namespace defines an enumerator "None". This clashes with a
// definition of "None" in X.h which is included by FLTK. So if there is any
// module including FLTK and YAML headers, things get nasty. Thus this clumsy
// attempt at hiding the YAML-cpp internals.
struct cfg_tileserver
{
    std::string name;
    std::string url;
    int zmin;
    int zmax;
    int parallel;
};

// Forward declaration of YAML-cpp node container
class yaml_node;

// A configuration node
class node
{
    public:
        node(yaml_node *in) : m_ref(in) {};
        node(const std::string& path);
        ~node();

        node operator[] (const int idx);
        node operator[] (const std::string &name);
        template<typename T> node& operator= (const T& rhs);
        
        template<typename T> T as() const;
        template<typename T> void push_back(const T& rhs);
		void push_back(const node& rhs);

        void serialize(const std::string& path);
    private:
        yaml_node *m_ref;
};

// Settings singleton
class settings
{
    public:
        ~settings();
        static settings& get_instance();

        node operator[] (const int idx) { return m_rootnode[idx]; };
        node operator[] (const std::string &name) { return m_rootnode[name]; };
    private:
        settings();
        node m_rootnode;
};

#endif // SETTINGS_HPP

