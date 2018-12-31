#include <FL/fl_ask.H>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "settings.hpp"
#include "utils.hpp"
#include "fluid/dlg_bulkdl.hpp"

void dlg_bulkdl::create_ex()
{
    // Set the window icon
    florb::utils::set_window_icon(m_window); 

    // Don't use default window callback
    m_window->callback(cb_window_ex);

    // Initialize UI
    m_progress_status->minimum(0);
    m_progress_status->maximum(99);
    m_input_zoomlevels->value("0;1;2-3");
    m_input_nice->value("1000");

    // Reset status
    cancel_ex(false);
    active_ex(false);

    // Register event handler for osmlayer notify events
    register_event_handler<dlg_bulkdl, florb::osmlayer::event_notify>(this, &dlg_bulkdl::osm_evt_notify_ex);
}

bool dlg_bulkdl::show_ex()
{
    // Populate the map selector
    m_choice_map->clear();
    YAML::Node section = florb::settings::get_instance()["tileservers"];
    for(YAML::Node::iterator it=section.begin(); it!=section.end(); ++it) {
        m_choice_map->add((*it).as<florb::cfg_tileserver>().name().c_str());
    }
    m_choice_map->value(0);

    // Initialize progress bar
    m_progress_status->value(0);
    m_progress_status->label("0 %");

    // Show the window
    m_window->show();
    bool ok = false;
    for (;;) {
        Fl_Widget *o = Fl::readqueue();
        if (!o) Fl::wait();
        else if (o == m_btn_ok)     {ok=true; break;}
        else if (o == m_window)     
        {
            if (!active_ex()) 
                break;    
        }
    }

    // Hide when done
    m_window->hide();

    return ok;
}

std::vector<unsigned int> dlg_bulkdl::parse_zoomlevels_ex()
{
    bool rc = true;
    std::vector<unsigned int> ret;

    //  Split the string at separator
    std::string desc(m_input_zoomlevels->value());
    florb::utils::str_replace(desc, " ", "");
    std::vector< std::string > tokens = florb::utils::str_split(desc, ";");
   
    // Look at each token
    for (std::vector< std::string >::iterator it=tokens.begin();it!=tokens.end();++it)
    {
        for (;;)
        {
            // Try to convert the token to an int first
            unsigned int conv1, conv2;
            if (florb::utils::fromstr((*it), conv1))
            {
                ret.push_back(conv1);
                break;
            }

            // Try to split the token and construct a range
            std::size_t c = florb::utils::str_count((*it), "-");
            std::vector< std::string > r = florb::utils::str_split((*it), "-");
            if ((r.size() == 2) && (c == 1))
            {
               // Try to convert upper and lower bounds to int
               if (florb::utils::fromstr(r[0], conv1) && florb::utils::fromstr(r[1], conv2))
               {
                    // Add every zoom level in the range
                    for (
                        unsigned int i = (conv1 > conv2) ? conv2 : conv1; 
                        i <= ((conv1 > conv2) ? conv1 : conv2);
                        i++)
                    {
                        ret.push_back(i);
                    }

                    break;
               }
            }

            rc = false;
            break;
        }

        // Don't even bother going on
        if (!rc)
            break;
    }

    // Error
    if ((!rc) || (ret.size() == 0))
    {
        throw std::runtime_error(_("Failure parsing zoomlevel description."));
    }

    // Remove duplicates
    std::sort(ret.begin(), ret.end());
    ret.erase(
      std::unique(ret.begin(), ret.end()),
      ret.end());

    // Check for zoom levels greater or smaller than what the tileserver supports
    florb::cfg_tileserver cfgtileserver = 
        florb::settings::get_instance()["tileservers"][m_choice_map->value()].as<florb::cfg_tileserver>();
    for (std::vector<unsigned int>::iterator it=ret.begin();it!=ret.end();++it)
    {
        if (((*it) < cfgtileserver.zmin()) ||
            ((*it) > cfgtileserver.zmax())) 
        {
            rc = false;
            break;
        }
    }

    // Error
    if (!rc)
    {
        throw std::runtime_error(_("Invalid zoomlevel specified for this map."));
    }

    return ret;
}

void dlg_bulkdl::cb_btn_download_ex(Fl_Widget *widget)
{
    // Cancel button mode
    if (active_ex())
    {
        cancel_ex(true);
        return;
    }

    // Parse requested zoom levels
    std::vector<unsigned int> zoomlevels;
    try {
        m_zoomlevels = parse_zoomlevels_ex();
    } catch (std::runtime_error& e) {
        fl_alert("%s", e.what());
        return;
    }

    // Update UI
    m_btn_download->label(_("Cancel"));
    m_choice_map->deactivate();
    m_input_zoomlevels->deactivate();
    m_input_nice->deactivate();
    m_btn_ok->deactivate();

    // Start downloading
    active_ex(true);
    Fl::awake(cb_startdl_ex, this);
}

void dlg_bulkdl::cb_startdl_ex(void *userdata)
{
    dlg_bulkdl *d = static_cast<dlg_bulkdl*>(userdata);
    d->startdl_ex();
}

void dlg_bulkdl::cb_window_ex(Fl_Widget *w, void *userdata)
{
    // The default window callback hides the window (which is not what we want)
    // and then calls the default widget callback. Use this to skip the
    // undesired hide().
    Fl_Widget::default_callback(w, userdata);
}

void dlg_bulkdl::startdl_ex()
{
    static std::size_t levelidx = 0;
    static florb::osmlayer *osml = NULL;

    // All done or canceled. Clean up and reset UI
    if ((levelidx >= m_zoomlevels.size()) || (cancel_ex()))
    {
        // Reset zoom level index
        levelidx = 0;

        // Reset UI
        if (!cancel_ex())
        {
            m_progress_status->label("100 %");
            m_progress_status->value(100);
        }
        else
        {
            m_progress_status->label("0 %");
            m_progress_status->value(0);
        }

        m_btn_download->label(_("Download"));
        m_choice_map->activate();
        m_input_zoomlevels->activate();
        m_input_nice->activate();
        m_btn_ok->activate();

        // Reset status
        active_ex(false);
        cancel_ex(false);

        // Destroy the osmlayer
        if (osml != NULL) {
            delete osml;
            osml = NULL;
        }

        return;
    }

    // Create the osmlayer to perform the downloading
    if (osml == NULL)
    {
        bool rc = true;

        for (;;)
        {
            florb::cfg_tileserver cfgtileserver = 
                florb::settings::get_instance()["tileservers"][m_choice_map->value()].as<florb::cfg_tileserver>();

            try {
                osml = new florb::osmlayer(
                    cfgtileserver.name(), 
                    cfgtileserver.url(), 
                    cfgtileserver.zmin(), 
                    cfgtileserver.zmax(), 
                    1,
                    cfgtileserver.type());
            } catch (std::runtime_error &e) {
                fl_alert("%s", e.what());
                osml = NULL;
                rc = false;
                break;
            }

            // Set nice value
            long ms;
            if (!florb::utils::fromstr(m_input_nice->value(), ms) || (ms < 0))
            {
                fl_alert("%s", _("Invalid delay"));
                rc = false;
                break;
            }
            osml->nice(ms);

            // Start listening to events
            osml->add_event_listener(this);
        
            break;
        }

        // Could not create / initialize osmlayer
        if (!rc)
        {
            cancel_ex(true);
            Fl::awake(cb_startdl_ex, this);
            return;
        }
    }

    // Calculate mercator coordinates for the requested download viewport and
    // convert them back for the current zoom level
    florb::point2d<double> pmerc1(florb::utils::px2merc(m_vp.z(), florb::point2d<unsigned long>(m_vp.x(), m_vp.y())));
    florb::point2d<double> pmerc2(florb::utils::px2merc(m_vp.z(), florb::point2d<unsigned long>(m_vp.x()+m_vp.w(), m_vp.y()+m_vp.h())));
    florb::point2d<unsigned long> ppx1(florb::utils::merc2px(m_zoomlevels[levelidx], pmerc1));
    florb::point2d<unsigned long> ppx2(florb::utils::merc2px(m_zoomlevels[levelidx], pmerc2));

    // Generate a viewport for the respective zoom level. Every member
    // (x/y/w/h) is initialized separately to make use of the viewport's bounds
    // checking, which the constructor does not do (yet).
    florb::viewport vp_tmp(0,0,m_zoomlevels[levelidx],0,0);
    vp_tmp.w(((ppx2.x() - ppx1.x()) == 0) ? 1 : (ppx2.x() - ppx1.x()));
    vp_tmp.h(((ppx2.y() - ppx1.y()) == 0) ? 1 : (ppx2.y() - ppx1.y()));
    vp_tmp.x(ppx1.x());
    vp_tmp.y(ppx1.y());

    static std::string plabel;
    std::ostringstream oss;
    oss << _("Zoom ") << m_zoomlevels[levelidx] << ": ";

    // Tell the layer to download this viewport. On success proceed to the next
    // requested zoom level
    double coverage;
    if (osml->download(vp_tmp, coverage))
    {
        // Reset UI
        oss << "100 %";
        plabel = oss.str();
        m_progress_status->label(plabel.c_str());
        m_progress_status->value(100);

        // Increas the zoom level index and start over
        levelidx++;
        Fl::awake(cb_startdl_ex, this);
        return;
    }

    // The current map is still incomplete, update UI with current status
    oss << (int)(coverage*100.0) << " %";
    plabel = oss.str();

    m_progress_status->label(plabel.c_str());
    m_progress_status->value((int)(coverage*100.0));
}

bool dlg_bulkdl::osm_evt_notify_ex(const florb::osmlayer::event_notify *e)
{
    // osmlayer has new tiles
    Fl::awake(cb_startdl_ex, this);
    return true;
}

bool dlg_bulkdl::cancel_ex()
{
    m_mutex.lock();
    bool ret = m_cancel;
    m_mutex.unlock();

    return ret;
}

void dlg_bulkdl::cancel_ex(bool c)
{
    m_mutex.lock();
    m_cancel = c;
    m_mutex.unlock();
}

bool dlg_bulkdl::active_ex()
{
    m_mutex.lock();
    bool ret = m_active;
    m_mutex.unlock();

    return ret;
}

void dlg_bulkdl::active_ex(bool a)
{
    m_mutex.lock();
    m_active = a;
    m_mutex.unlock();
}

