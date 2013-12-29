#include <sstream>
#include "settings.hpp"
#include "fluid/dlg_gpsd.hpp"

void dlg_gpsd::create_ex()
{
    bool enable = settings::get_instance()["gpsd"]["enabled"].as<bool>();

    if (enable)
        m_chkbtn_enable->value(1);
    else
        m_chkbtn_enable->value(0);
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
    m_window->show();

    std::string host(settings::get_instance()["gpsd"]["host"].as<std::string>());
    std::string port(settings::get_instance()["gpsd"]["port"].as<std::string>());

    m_txtin_server->value(host.c_str());
    m_txtin_port->value(port.c_str());

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
    host = std::string(m_txtin_server->value());
    port = std::string(m_txtin_port->value());

    settings::get_instance()["gpsd"]["host"] = host;
    settings::get_instance()["gpsd"]["port"] = port;

    if (m_chkbtn_enable->value() == 0)
    {
        settings::get_instance()["gpsd"]["enabled"] = false;
        m_mapctrl->disconnect();
    }
    else
    {
        settings::get_instance()["gpsd"]["enabled"] = true;
        m_mapctrl->connect(host, port); 
    }

    m_window->hide();
}

void dlg_gpsd::cb_chkbtn_enable_ex(Fl_Widget *widget)
{
    ui_setup_ex();
}

