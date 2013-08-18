#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <map>

class settings
{
    public:
        ~settings();
        static settings& get_instance();
        void serialize();

        template <class T>
            bool getopt(const std::string& section, const std::string& key, T& t)
            {
                std::map<std::string,std::string>::iterator it;

                // Check if section exists at all
                smap_t::iterator sit;
                sit = m_settings.find(section);

                // No, exit
                if (sit == m_settings.end())
                    return false;

                // Try to find the key
                kvmap_t::iterator kvit;
                kvit = m_settings[section].find(key);

                // Not found, exit
                if (kvit == m_settings[section].end())
                    return false;

                // Convert and return value
                std::istringstream iss(m_settings[section][key]);
                if ((iss >> t).fail())
                    return false;

                return true;
            }

        template <class T>
            void setopt(const std::string& section, const std::string& key, const T& val)
            {
                // Convert whatever type we got to string
                std::ostringstream oss;
                oss << val;

                // Check if section already exists
                smap_t::iterator sit;
                sit = m_settings.find(section);

                // No, create
                if (sit == m_settings.end())
                    m_settings.insert(make_pair(section, kvmap_t()));

                // Check if key already exists
                kvmap_t::iterator kvit;
                kvit = m_settings[section].find(key);

                // Yes, update value
                if (kvit != m_settings[section].end())
                    m_settings[section][key] = oss.str();
                // No, insert new key-value pair
                else
                    m_settings[section].insert(make_pair(key, oss.str()));

                return true;
            }

    private:
        typedef std::map<std::string, std::string> kvmap_t;
        typedef std::map<std::string, kvmap_t> smap_t;

        static const std::string m_separator;
        std::string m_path;
        smap_t m_settings;

        settings();
        //settings(const settings&);

        void marshal(void);
        std::string strip(const std::string &str, const std::string &what = " ");
};

#endif // SETTINGS_HPP

