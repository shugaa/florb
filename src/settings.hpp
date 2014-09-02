#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <iterator>
#include "gfx.hpp"
#include "utils.hpp"
#include "unit.hpp"

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
        cfg_tileserver() : 
            m_name("OpenStreetMap"),
            m_url("http://tile.openstreetmap.org/{z}/{x}/{y}.png"),
            m_zmin(0),
            m_zmax(18),
            m_parallel(2),
            m_type(florb::image::PNG) {};

        const std::string& name() const { return m_name; }
        void name(const std::string& n) { m_name = n; }

        const std::string& url() const { return m_url; }
        void url(const std::string& u) { m_url = u; }

        unsigned int zmin() const { return m_zmin; }
        void zmin(unsigned int z) { m_zmin = z; }

        unsigned int zmax() const { return m_zmax; }
        void zmax(unsigned int z) { m_zmax = z; }

        unsigned int parallel() const { return m_parallel; }
        void parallel(unsigned int p) { m_parallel = p; }

        unsigned int type() const { return m_type; }
        void type(unsigned int t) { m_type = t; }

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
        cfg_gpsd() :
            m_enabled(false),
            m_host("localhost"),
            m_port("2947") {};

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
        cfg_cache() :
            m_location(utils::appdir() + "/tiles") {};
    
        const std::string& location() const { return m_location; }
        void location(const std::string& location) { m_location = location; }

    private:    
        std::string m_location; 
};

// UI configuration class
class cfg_ui
{
    public:
        cfg_ui() :
            m_markercolor(florb::color(0,0,0xff)),
            m_markercolorselected(florb::color(0,0xff,0)),
            m_trackcolor(florb::color(0xff,0,0)),
            m_selectioncolor(florb::color(0xff,0,0xff)),
            m_gpscursorcolor(florb::color(0xff,0,0xff)),
            m_tracklinewidth(2) {};
    
        florb::color markercolor() const { return m_markercolor; }
        void markercolor(florb::color c) { m_markercolor = c; }

        florb::color markercolorselected() const { return m_markercolorselected; }
        void markercolorselected(florb::color c) { m_markercolorselected = c; }

        florb::color trackcolor() const { return m_trackcolor; }
        void trackcolor(florb::color c) { m_trackcolor = c; }

        florb::color selectioncolor() const { return m_selectioncolor; }
        void selectioncolor(florb::color c) { m_selectioncolor = c; }

        florb::color gpscursorcolor() const { return m_gpscursorcolor; }
        void gpscursorcolor(florb::color c) { m_gpscursorcolor = c; }

        unsigned int tracklinewidth() const { return m_tracklinewidth; }
        void tracklinewidth(unsigned int w) { m_tracklinewidth = w; }

    private:

        florb::color m_markercolor;
        florb::color m_markercolorselected;
        florb::color m_trackcolor;
        florb::color m_selectioncolor;
        florb::color m_gpscursorcolor;
        unsigned int m_tracklinewidth;

};

// Viewport configuration class
class cfg_viewport
{
    public:
        cfg_viewport() :
            m_lon(0),
            m_lat(0),
            m_z(0) {};
    
        double lat() const { return m_lat; }
        double lon() const { return m_lon; }
        unsigned int z() const { return m_z; }

        void lat(double l) { m_lat = l; }
        void lon(double l) { m_lon = l; }
        void z(unsigned int z) { m_z = z; }

    private:    
        double m_lon;
        double m_lat;
        unsigned int m_z;
};

// Units configuration class
class cfg_units
{
    public:

        enum system
        {
            METRIC,
            IMPERIAL,
            NAUTICAL,

            /* Always last */
            ENUM_SYSTEM_END
        };

        cfg_units() :
            m_sl(system::METRIC) {};
   

        system system_length() const { return m_sl; }
        void system_length(system sl) { m_sl = sl; }

    private:    
        system m_sl; 
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

        bool is_sequence();

        node operator[] (const int idx);
        node operator[] (const std::string &name);
        node& operator= (const node& n);
        template<typename T> node& operator= (const T& rhs);

        explicit operator bool() const;

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
        settings(const settings& s);
        void defaults(const std::string& path);
        node m_rootnode;
        std::string m_cfgfile;
};

#endif // SETTINGS_HPP

