#include <fstream>

#include "settings.hpp"

const std::string settings::m_separator = ":";

settings::settings()
{
    // TODO: Configuration directory?
    m_path = "/home/bjoern/florb.cfg";

    // Load config file
    marshal();
};

settings::~settings()
{
    serialize();
};

void settings::serialize()
{
    std::ofstream file(m_path.c_str(), std::ios_base::out|std::ios_base::trunc);

    smap_t::iterator sit;
    for (sit = m_settings.begin(); sit != m_settings.end(); ++sit) {

        // Separate sections by newline
        if (sit != m_settings.begin())
            file << "\n";

        // Write section header
        file << "[" << (*sit).first << "]" << "\n";

        // Write key value pairs
        kvmap_t::iterator kvit;
        for (kvit = (*sit).second.begin(); kvit != (*sit).second.end(); ++kvit) {
            file 
                << (*kvit).first 
                << " " << m_separator << " "
                << (*kvit).second
                << "\n";
        }
    }
}

void settings::marshal(void)
{
    std::ifstream file(m_path.c_str());
    std::string line;
    std::string section;

    while (std::getline(file,line)) {

        // Remove any whitespace on the line
        line = strip(line);

        // Empty line
        if (!line.length()) continue;

        // Section header
        if (line[0] == '[') {
            section = line.substr(1, line.length()-2);
            m_settings.insert(make_pair(section, kvmap_t()));
            continue;
        }

        // Key-Value pair
        size_t pos;
        if ((pos = line.find(m_separator)) != std::string::npos) {
            std::string key = strip(line.substr(0,pos));
            std::string value = strip(line.substr(pos+1,line.length()));
            m_settings[section].insert(make_pair(key, value));
        }

        // Anything else
    }
}

std::string settings::strip(const std::string &str, const std::string &what)
{
    std::string tmp(str);

    // Strip front
    while (tmp.find(what) == 0)
        tmp.erase(0, what.length());

    // Strip back
    size_t pos;
    while (((pos = tmp.rfind(what)) == (tmp.length()-what.length())) && (pos != std::string::npos))
        tmp.erase(pos, what.length());

    return tmp;
}

settings& settings::get_instance()
{
    static settings instance;
    return instance;
}

