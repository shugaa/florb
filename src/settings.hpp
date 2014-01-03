#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <iterator>

// The YAML namespace defines an enumerator "None". This clashes with a
// definition of "None" in X.h which is included by FLTK. So if there is any
// module including FLTK and YAML headers, things get nasty. Thus this clumsy
// attempt at hiding the YAML-cpp internals. It could have been so easy...
// On a positive note it is now relatively easy to swap out YAML-cpp for some
// other backend.

// Tileserver configuration class
class cfg_tileserver
{
    public:
        cfg_tileserver() {};
        cfg_tileserver(
                const std::string& name,
                const std::string& url,
                unsigned int zmin,
                unsigned int zmax,
                unsigned int parallel,
                int type) :
            m_name(name),
            m_url(url),
            m_zmin(zmin),
            m_zmax(zmax),
            m_parallel(parallel),
            m_type(type) {};

        const std::string& name() const { return m_name; };
        void name(const std::string& n) { m_name = n; }

        const std::string& url() const { return m_url; };
        void url(const std::string& u) { m_url = u; }

        unsigned int zmin() const { return m_zmin; };
        void zmin(unsigned int z) { m_zmin = z; };

        unsigned int zmax() const { return m_zmax; };
        void zmax(unsigned int z) { m_zmax = z; };

        unsigned int parallel() const { return m_parallel; };
        void parallel(unsigned int p) { m_parallel = p; };

        unsigned int type() const { return m_type; };
        void type(unsigned int t) { m_type = t; };

    private:
        std::string m_name;
        std::string m_url;
        unsigned int m_zmin;
        unsigned int m_zmax;
        unsigned int m_parallel;
        int m_type;
};

// GPSd configuration class
class cfg_gpsd
{
    public:
        cfg_gpsd() {};
        cfg_gpsd(bool enabled, const std::string& host, const std::string& port) :
            m_enabled(enabled),
            m_host(host),
            m_port(port) {};

        bool enabled() const { return m_enabled; };
        void enabled(bool e) { m_enabled = e; };

        const std::string& host() const { return m_host; };
        void host(const std::string& h) { m_host = h; };

        const std::string& port() const { return m_port; };
        void port(const std::string& p) { m_port = p; };

    private:
        bool m_enabled;
        std::string m_host;
        std::string m_port;
};

// Cache configuration class
class cfg_cache
{
    public:
        cfg_cache() {};
        cfg_cache(const std::string& location) :
            m_location(location) {};
    
        const std::string& location() const { return m_location; }
        void location(const std::string& location) { m_location = location; };

    private:    
        std::string m_location; 
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

        node();
        node(yaml_node *in) : m_ref(in) {};
        node(const std::string& path);
        node(const node& n);
        ~node();

        node operator[] (const int idx);
        node operator[] (const std::string &name);
        node& operator= (const node& n);
        template<typename T> node& operator= (const T& rhs);

        size_t size();
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
        void defaults(const std::string& path);
        node m_rootnode;
        std::string m_cfgfile;
};

#endif // SETTINGS_HPP

