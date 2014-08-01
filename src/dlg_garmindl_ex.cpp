#include <FL/fl_ask.H>
#include <sstream>
#include <iostream>
#include "utils.hpp"
#include "shell.hpp"
#include "fluid/dlg_garmindl.hpp"

void dlg_garmindl::create_ex()
{
    m_dlstatus = false;

    // Set the window icon
    utils::set_window_icon(m_window); 

    m_progress_status->minimum(0);
    m_progress_status->maximum(99);
    m_progress_status->value(0);
    m_progress_status->label("0 %");
}

bool dlg_garmindl::show_ex()
{
    shell sh;
    sh.run("gpsbabel -i garmin -f usb:-1");

    m_choice_device->clear();

    std::string tmp;
    while(sh.readln(tmp))
    {
        std::cout << tmp << std::endl;

        // Very basic gpsbabel output validation (check first item integer)
        std::istringstream conv(utils::str_split(tmp, " ")[0]);
        int devidx;
        conv >> devidx;
        if ((conv.rdstate() & (std::istringstream::failbit|std::istringstream::badbit)) != 0)
            break;

        m_choice_device->add(tmp.c_str());
    }

    sh.wait();

    if (m_choice_device->size() == 0)
    {
        m_choice_device->add(_("No devices found"));
        m_btn_download->deactivate();
        m_choice_device->deactivate();
    }
    else
    {
        m_btn_download->activate();
        m_choice_device->activate();
    }

    m_choice_device->value(0);

    // Show the window
    m_window->show();
    bool ok = false;
    for (;;) {
        Fl_Widget *o = Fl::readqueue();
        if (!o) Fl::wait();
        else if (o == m_btn_ok)     {ok=true; break;}
        else if (o == m_btn_cancel) {break;}
        else if (o == m_window)     {break;}
    }

    if (!m_dlstatus)
        ok = false;

    m_window->hide();

    return ok;
}

void dlg_garmindl::cb_btn_download_ex(Fl_Widget *widget)
{
    m_btn_download->deactivate();
    m_btn_ok->deactivate();
    m_btn_cancel->deactivate();
    m_choice_device->deactivate();

    m_dlstatus = false;

    std::ostringstream os;
    int idx = m_choice_device->value();
    os << "gpsbabel -vs -t -i garmin -f usb:" << idx << " -x track,name=\"ACTIVE*\" -o gpx -F " << m_path;

    shell sh;
    sh.run(os.str());

    std::string line;
    while (sh.readln(line))
    {
        std::vector<std::string> progress = utils::str_split(line, "/");
        if (progress.size() != 3)
            continue;

        std::istringstream is(progress[0]);
        float v;
        is >> v;
        m_progress_status->value(v);

        progress[0] += " %";
        m_progress_status->label(progress[0].c_str());
        Fl::check();
    }

    m_progress_status->value(100);
    m_progress_status->label("100 %");

    m_btn_download->activate();
    m_btn_ok->activate();
    m_btn_cancel->activate();
    m_choice_device->activate();

    if (sh.wait())
    {
        fl_message("%s", _("Download OK"));
        m_dlstatus = true;
    }
    else
        fl_message("%s", _("Download Failed!"));
}

