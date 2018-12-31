#include <yaml-cpp/yaml.h>
#include <fstream>
#include "settings.hpp"

florb::settings::settings()
{
    m_cfgfile = florb::utils::appdir() + florb::utils::pathsep() + "config";
    florb::utils::mkdir(florb::utils::appdir());

    m_rootnode = YAML::LoadFile(m_cfgfile);
    defaults(m_cfgfile);
}

florb::settings::~settings()
{
    std::ofstream fout(m_cfgfile.c_str(), std::fstream::in|std::fstream::out|std::fstream::trunc);
    fout << m_rootnode;
}

florb::settings& florb::settings::get_instance()
{
    static florb::settings instance;
    return instance;
}

void florb::settings::defaults(const std::string& path)
{
    // Tileserver default configuration
    florb::cfg_tileserver cfgtileserver;
    if((!m_rootnode["tileservers"]) || (!m_rootnode["tileservers"].IsSequence()))
    {
        std::vector<florb::cfg_tileserver> v;
        v.push_back(cfgtileserver);

        m_rootnode["tileservers"] = v;
    }

    // GPSd default configuration
    florb::cfg_gpsd cfggpsd = m_rootnode["gpsd"].as<florb::cfg_gpsd>();
    m_rootnode["gpsd"] = cfggpsd;

    // Cache default configuration
    florb::cfg_cache cfgcache = m_rootnode["cache"].as<florb::cfg_cache>();
    m_rootnode["cache"] = cfgcache;

    // UI default configuration
    florb::cfg_ui cfgui = m_rootnode["ui"].as<florb::cfg_ui>();
    m_rootnode["ui"] = cfgui;

    // Viewport default configuration
    florb::cfg_viewport cfgvp = m_rootnode["viewport"].as<florb::cfg_viewport>();
    m_rootnode["viewport"] = cfgvp;

    // Units default configuration
    florb::cfg_units cfgunits = m_rootnode["units"].as<florb::cfg_units>();
    m_rootnode["units"] = cfgunits;
}

