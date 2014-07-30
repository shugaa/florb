#include "settings.hpp"
#include "utils.hpp"
#include "fluid/dlg_tileserver.hpp"
#include <FL/fl_ask.H>

void dlg_tileserver::create_ex()
{
    // Set the window icon
    utils::set_window_icon(m_window); 
    m_window->label(m_title.c_str());
}

bool dlg_tileserver::show_ex()
{
    // Populate the image type selection widget
    m_choice_imgtype->add("PNG");
    m_choice_imgtype->add("JPEG");

    switch (m_cfgtileserver.type())
    {
        case fgfx::image::PNG:
            m_choice_imgtype->value(0);
            break;
        case fgfx::image::JPG:
            m_choice_imgtype->value(1);
            break;
        default:
            m_choice_imgtype->value(0);
    }

    // Populate the other widgets
    m_input_name->value(m_cfgtileserver.name().c_str());
    m_input_name->position(0,0);
    m_input_url->value(m_cfgtileserver.url().c_str());
    m_input_url->position(0,0);
    m_spinner_zmin->value((double)m_cfgtileserver.zmin());
    m_spinner_zmax->value((double)m_cfgtileserver.zmax());
    m_spinner_parallel->value((double)m_cfgtileserver.parallel());

    // Show the window
    m_window->show();

    bool ok = false;
    for (;;) {
        Fl_Widget *o = Fl::readqueue();
        if (!o) Fl::wait();
        else if (o == m_btn_ok)     
        {
             // Make sure name is not empty
            if (strlen(m_input_name->value()) == 0)
            {
                fl_alert("%s", _("Please provide a name for this server!"));
                continue;
            }

            // Make sure url is not empty
            if (strlen(m_input_url->value()) == 0)
            {
                fl_alert("%s", _("Please specify the download url for this server!"));
                continue;
            }
            
            ok=true;
            break;
        }
        else if (o == m_btn_cancel) {break;}
        else if (o == m_window)     {break;}
    }

    // OK button, save the form data
    if (ok)
    {
        switch (m_choice_imgtype->value())
        {
            case 0:
                m_cfgtileserver.type(fgfx::image::PNG);
                break;
            case 1:
                m_cfgtileserver.type(fgfx::image::JPG);
                break;
            default:
                m_cfgtileserver.type(fgfx::image::PNG);
        }

        m_cfgtileserver.name(m_input_name->value());
        m_cfgtileserver.url(m_input_url->value());
        m_cfgtileserver.zmin(m_spinner_zmin->value());
        m_cfgtileserver.zmax(m_spinner_zmax->value());
        m_cfgtileserver.parallel(m_spinner_parallel->value());
    }

    m_window->hide();
    return ok;
}

cfg_tileserver dlg_tileserver::tileserver_ex()
{
    // Return a copy of the local confuguratin data for the tileserver
    return m_cfgtileserver;
}

void dlg_tileserver::cb_spinner_zmin_ex(Fl_Widget *widget)
{
    // Make sure zmax is greater than zmin
    if (m_spinner_zmax->value() <= m_spinner_zmin->value())
        m_spinner_zmax->value(m_spinner_zmin->value()+1);
}

void dlg_tileserver::cb_spinner_zmax_ex(Fl_Widget *widget)
{
    // Make sure zmax is greater than zmin
    if (m_spinner_zmax->value() <= m_spinner_zmin->value())
        m_spinner_zmin->value(m_spinner_zmax->value()-1);
}

