#include <string>
#include <sstream>
#include <tinyxml2.h>
#include <clocale>
#include <limits>
#include "utils.hpp"
#include "fluid/dlg_search.hpp"

void dlg_search::create_ex()
{
    m_markerid = std::numeric_limits<size_t>::max();

    // Set the window icon
    florb::utils::set_window_icon(m_window);

    try {
        m_downloader = new florb::downloader(1);
    } catch (std::runtime_error& e) {
        m_window->hide();
        throw e;
    }

    m_downloader->timeout(20);
    register_event_handler<dlg_search, florb::downloader::event_complete>(this, &dlg_search::evt_downloadcomplete_ex);
    m_downloader->add_event_listener(this);
}

void dlg_search::destroy_ex()
{
    if (m_downloader)
        delete m_downloader;

    if (m_window)
        delete m_window;
}

void dlg_search::cb_btn_search_ex(Fl_Widget *widget)
{
    std::string query(m_input_query->value());
    if (query.length() == 0)
        return;

    if (m_markerid != std::numeric_limits<size_t>::max())
    {
        m_wgtmap->marker_remove(m_markerid);
        m_markerid = std::numeric_limits<size_t>::max();
    }

    // Perform some very basic string sanitation
    size_t pos;
    while((pos = query.find("?")) != std::string::npos)
        query.replace(pos, 1, "");
    while((pos = query.find("/")) != std::string::npos)
        query.replace(pos, 1, "");
    while((pos = query.find("\\")) != std::string::npos)
        query.replace(pos, 1, "");
    while((pos = query.find("=")) != std::string::npos)
        query.replace(pos, 1, "");

    m_btn_search->deactivate();
    m_btn_ok->deactivate();
    m_input_query->deactivate();
    m_browser_results->deactivate();
    m_browser_results->clear();
    m_searchresults.clear();

    // Construct query string
    std::ostringstream oss;
    oss << "http://nominatim.openstreetmap.org/search/"; 
    oss << query;
    oss << "?format=xml";

    // Download the search results
    bool ret = m_downloader->queue(oss.str(), NULL);
    if (!ret)
    {
        m_btn_search->activate();
        m_btn_ok->activate();
        m_input_query->activate();
        m_browser_results->activate();
    }
}

void dlg_search::cb_browser_results_ex(Fl_Widget *widget)
{
    int v = m_browser_results->value();
    if (v == 0)
    {
        if (m_markerid != std::numeric_limits<size_t>::max())
        {
            m_wgtmap->marker_remove(m_markerid);
            m_markerid = std::numeric_limits<size_t>::max();
        }

        return;
    }

    if (m_markerid != std::numeric_limits<size_t>::max())
        m_wgtmap->marker_remove(m_markerid);

    m_markerid = m_wgtmap->marker_add(florb::utils::wsg842merc(m_searchresults[v-1].m_pos));
    m_wgtmap->goto_pos(m_searchresults[v-1].m_pos);
}

void dlg_search::cb_btn_ok_ex(Fl_Widget *widget)
{
    hide_ex();
}

void dlg_search::cb_window_ex(Fl_Widget *widget)
{
    hide_ex();
}

void dlg_search::show_ex()
{
    // Focus on query input
    m_input_query->position(0, std::string(m_input_query->value()).length());
    m_input_query->take_focus();

    // Show the window
    m_window->show();
}

void dlg_search::hide_ex()
{
    if (m_markerid != std::numeric_limits<size_t>::max())
    {
        m_wgtmap->marker_remove(m_markerid);
        m_markerid = std::numeric_limits<size_t>::max();
    }

    // hide the window
    m_window->hide();
}

void dlg_search::process_download_ex()
{
    // TinyXML's number parsing is locale dependent, so we switch to "C"
    // and back after parsing
    char *poldlc;
    char oldlc[16];
    poldlc = setlocale(LC_ALL, NULL);
    strncpy(oldlc, poldlc, 15);
    oldlc[15] = '\0';
    setlocale(LC_ALL, "C");

    // Parse response
    for (;;)
    {
        florb::downloader::download dtmp;
        if (!m_downloader->get(dtmp))
            break;

        tinyxml2::XMLDocument doc;
        if (doc.Parse(std::string(dtmp.buf().begin(), dtmp.buf().end()).c_str()) != tinyxml2::XML_SUCCESS)
            break;

        tinyxml2::XMLElement* root = doc.RootElement();
        if (!root)
            break;

        if (std::string(root->ToElement()->Value()).compare("searchresults") != 0)
            break;

        tinyxml2::XMLNode *child;
        for (child = root->FirstChild(); child != NULL; child = child->NextSibling()) 
        {
            if (std::string(child->ToElement()->Value()).compare("place") != 0)
                continue;

            const char *dn = child->ToElement()->Attribute("display_name");
            if (dn == NULL)
                continue;

            double lat = 1234.5, lon = 1234.5;
            child->ToElement()->QueryDoubleAttribute("lat", &lat);
            child->ToElement()->QueryDoubleAttribute("lon", &lon);

            // Check for error
            if ((lat == 1234.5) || (lon == 1234.5))
                continue;

            searchresult tmp;
            tmp.m_displayname = std::string(dn);
            tmp.m_pos = florb::point2d<double>(lon,lat);
            
            m_searchresults.push_back(tmp);
            m_browser_results->add(dn);
        }

        break;
    }

    // Switch back to original locale
    setlocale(LC_ALL, oldlc);

    m_btn_search->activate();
    m_btn_ok->activate();
    m_input_query->activate();
    m_browser_results->activate();
}

void dlg_search::cb_download_ex(void *userdata)
{
    // This might be a callback for an already destroyed layer instance
    dlg_search *d = static_cast<dlg_search*>(userdata);
    d->process_download_ex();
}

bool dlg_search::evt_downloadcomplete_ex(const florb::downloader::event_complete *e)
{
    Fl::awake(cb_download_ex, this);
    return true;
}

