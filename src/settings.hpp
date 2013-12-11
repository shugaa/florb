#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <iterator>

// The YAML namespace defines an enumerator "None". This clashes with a
// definition of "None" in X.h which is included by FLTK. So if there is any
// module including FLTK and YAML headers, things get nasty. Thus this clumsy
// attempt at hiding the YAML-cpp internals. It could have been so easy...
struct cfg_tileserver
{
    std::string name;
    std::string url;
    int zmin;
    int zmax;
    int parallel;
};

// Forward declaration of YAML-cpp node container and iterator container
class yaml_node;
class yaml_iterator;

// A configuration node
class node
{
    public:
        class iterator : public std::iterator<std::forward_iterator_tag, int>
        {
            public:
                iterator(const node &n, int be);
                ~iterator();
                bool operator==(iterator const& rhs) const; 
                bool operator!=(iterator const& rhs) const;
                iterator& operator++();
                iterator operator++(int); 
#if 0
                iterator& operator--(); 
                iterator operator--(int); 
#endif
                node operator* () const; 
            private:
                yaml_iterator *m_ref;
        };
        iterator begin() { return iterator(*this, -1); }
        iterator end() { return iterator(*this, 1); }  

        node(yaml_node *in) : m_ref(in) {};
        node(const std::string& path);
        node(const node& n);
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
        node& root() { return m_rootnode; };

        node operator[] (const int idx) { return m_rootnode[idx]; };
        node operator[] (const std::string &name) { return m_rootnode[name]; };
    private:
        settings();
        node m_rootnode;
};

#endif // SETTINGS_HPP

