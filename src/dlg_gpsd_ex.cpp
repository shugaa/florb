#include <sstream>
#include "settings.hpp"
#include "utils.hpp"
#include "fluid/dlg_gpsd.hpp"

void dlg_gpsd::create_ex()
{
    cfg_gpsd cfggpsd = settings::get_instance()["gpsd"].as<cfg_gpsd>();

    if (cfggpsd.enabled())
        m_chkbtn_enable->value(1);
    else
        m_chkbtn_enable->value(0);

    // Set the window icon
    utils::set_window_icon(m_window); 
}

void dlg_gpsd::ui_setup_ex()
{
    bool enable = (m_chkbtn_enable->value() == 0) ? false : true; 

    if (enable)
    {
        m_chkbtn_enable->value(1);
        m_box_server->activate();
        m_txtin_server->activate();
        m_box_port->activate();
        m_txtin_port->activate();
    } else
    {
        m_chkbtn_enable->value(0);
        m_box_server->deactivate();
        m_txtin_server->deactivate();
        m_box_port->deactivate();
        m_txtin_port->deactivate();
    }
}

void dlg_gpsd::show_ex()
{
    ui_setup_ex();
    
    cfg_gpsd cfggpsd = settings::get_instance()["gpsd"].as<cfg_gpsd>();
    m_txtin_server->value(cfggpsd.host().c_str());
    m_txtin_port->value(cfggpsd.port().c_str());

    m_window->show();
    bool ok = false;
    for (;;) {
        Fl_Widget *o = Fl::readqueue();
        if (!o) Fl::wait();
        else if (o == m_btn_ok)     {ok=true;break;}
        else if (o == m_btn_cancel) {break;}
        else if (o == m_window)     {break;}
    }

    // Not the OK / connect button
    if (!ok)
        return;

    // OK, store and connect / disconnect
    cfggpsd.host(std::string(m_txtin_server->value()));
    cfggpsd.port(std::string(m_txtin_port->value()));

    if (m_chkbtn_enable->value() == 0)
    {
        cfggpsd.enabled(false);
        m_mapctrl->disconnect();
    }
    else
    {
        cfggpsd.enabled(true);
        m_mapctrl->connect(cfggpsd.host(), cfggpsd.port()); 
    }
    
    settings::get_instance()["gpsd"] = cfggpsd;
    m_window->hide();
}

void dlg_gpsd::cb_chkbtn_enable_ex(Fl_Widget *widget)
{
    ui_setup_ex();
}

