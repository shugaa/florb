#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <yaml-cpp/yaml.h>
#include "gfx.hpp"
#include "utils.hpp"

namespace florb
{
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
                m_location(florb::utils::appdir() + "/tiles") {};

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

    // Settings singleton
    class settings
    {
        public:
            ~settings();
            static settings& get_instance();
            YAML::Node& root() { return m_rootnode; };
            YAML::Node operator[] (const int idx) { return m_rootnode[idx]; };
            YAML::Node operator[] (const std::string &name) { return m_rootnode[name]; };
        private:
            settings();
            settings(const settings& s);
            void defaults(const std::string& path);
            YAML::Node m_rootnode;
            std::string m_cfgfile;
    };

};

namespace YAML {
    template<>
        struct convert<florb::cfg_tileserver> {
            static Node encode(const florb::cfg_tileserver& rhs) {
                Node node;
                node["name"] = rhs.name();
                node["url"] = rhs.url();
                node["zmin"] = rhs.zmin();
                node["zmax"] = rhs.zmax();
                node["parallel"] = rhs.parallel();

                node["type"] = "PNG";
                if      (rhs.type() == florb::image::PNG)
                    node["type"] = "PNG";
                else if (rhs.type() == florb::image::JPG)
                    node["type"] = "JPG";

                return node;
            }

            static bool decode(const Node& node, florb::cfg_tileserver& rhs) 
            {
                if((!node.IsMap()) || (node.size() == 0))
                    return true;

                if (node["name"])
                    rhs.name(node["name"].as<std::string>());

                if (node["url"])
                    rhs.url(node["url"].as<std::string>());

                if (node["zmin"])
                    rhs.zmin(node["zmin"].as<int>());

                if (node["zmax"])
                    rhs.zmax(node["zmax"].as<int>());

                if (node["parallel"])
                    rhs.parallel(node["parallel"].as<int>());

                if (node["type"])
                {
                    rhs.type(florb::image::PNG);
                    std::string imgtype = node["type"].as<std::string>();
                    if      (imgtype.compare("PNG") == 0)
                        rhs.type(florb::image::PNG);
                    else if (imgtype.compare("JPG") == 0)
                        rhs.type(florb::image::JPG);
                }

                return true;
            }
        };

    template<>
        struct convert<florb::cfg_cache> {
            static Node encode(const florb::cfg_cache& rhs) {
                Node node;
                node["location"] = rhs.location();
                return node;
            }

            static bool decode(const Node& node, florb::cfg_cache& rhs) 
            {
                if((!node.IsMap()) || (node.size() == 0))
                    return true;

                if (node["location"])
                    rhs.location(node["location"].as<std::string>());

                return true;
            }
        };

    template<>
        struct convert<florb::cfg_units> {
            static Node encode(const florb::cfg_units& rhs) {
                Node node;

                switch (rhs.system_length())
                {
                    case (florb::cfg_units::system::IMPERIAL):
                        node["system_length"] = "imperial";
                        break;
                    case (florb::cfg_units::system::NAUTICAL):
                        node["system_length"] = "nautical";
                        break;
                    default:
                        node["system_length"] = "metric";
                        break;
                }

                return node;
            }

            static bool decode(const Node& node, florb::cfg_units& rhs) 
            {
                if((!node.IsMap()) || (node.size() == 0))
                    return true;

                if (node["system_length"])
                {
                    std::string sm(node["system_length"].as<std::string>());
                    if (sm == "imperial")
                        rhs.system_length(florb::cfg_units::system::IMPERIAL);
                    else if (sm == "nautical")
                        rhs.system_length(florb::cfg_units::system::NAUTICAL);
                    else
                        rhs.system_length(florb::cfg_units::system::METRIC);
                }

                return true;
            }
        };

    template<>
        struct convert<florb::cfg_gpsd> {
            static Node encode(const florb::cfg_gpsd& rhs) {
                Node node;
                node["enabled"] = rhs.enabled();
                node["host"] = rhs.host();
                node["port"] = rhs.port();
                return node;
            }

            static bool decode(const Node& node, florb::cfg_gpsd& rhs) 
            {
                if((!node.IsMap()) || (node.size() == 0))
                    return true;

                if (node["enabled"])
                    rhs.enabled(node["enabled"].as<bool>());

                if (node["host"])
                    rhs.host(node["host"].as<std::string>());

                if (node["port"])
                    rhs.port(node["port"].as<std::string>());

                return true;
            }
        };

    template<>
        struct convert<florb::cfg_ui> {
            static Node encode(const florb::cfg_ui& rhs) {
                Node node;
                node["markercolor"] = rhs.markercolor().rgb();
                node["markercolorselected"] = rhs.markercolorselected().rgb();
                node["trackcolor"] = rhs.trackcolor().rgb();
                node["selectioncolor"] = rhs.selectioncolor().rgb();
                node["gpscursorcolor"] = rhs.gpscursorcolor().rgb();
                node["tracklinewidth"] = rhs.tracklinewidth();
                return node;
            }

            static bool decode(const Node& node, florb::cfg_ui& rhs) 
            {
                // Use defaults
                if((!node.IsMap()) || (node.size() == 0))
                {
                    return true;
                }

                if (node["markercolor"])
                    rhs.markercolor(florb::color(node["markercolor"].as<unsigned int>()));

                if (node["markercolorselected"])
                    rhs.markercolorselected(florb::color(node["markercolorselected"].as<unsigned int>()));

                if (node["trackcolor"])
                    rhs.trackcolor(florb::color(node["trackcolor"].as<unsigned int>()));

                if (node["selectioncolor"])
                    rhs.selectioncolor(florb::color(node["selectioncolor"].as<unsigned int>()));

                if (node["gpscursorcolor"])
                    rhs.gpscursorcolor(florb::color(node["gpscursorcolor"].as<unsigned int>()));

                if (node["tracklinewidth"])
                    rhs.tracklinewidth(node["tracklinewidth"].as<unsigned int>());

                return true;
            }
        };

    template<>
        struct convert<florb::cfg_viewport> {
            static Node encode(const florb::cfg_viewport& rhs) {
                Node node;
                node["lon"] = rhs.lon();
                node["lat"] = rhs.lat();
                node["z"] = rhs.z();
                return node;
            }

            static bool decode(const Node& node, florb::cfg_viewport& rhs) 
            {
                if((!node.IsMap()) || (node.size() == 0))
                {
                    return true;
                }


                if (node["lat"])
                    rhs.lat(node["lat"].as<double>());

                if (node["lon"])
                    rhs.lon(node["lon"].as<double>());

                if (node["z"])
                    rhs.z(node["z"].as<unsigned int>());

                return true;
            }
        };
}


#endif // SETTINGS_HPP

