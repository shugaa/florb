#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include "utils.hpp"
#include "version.hpp"
#include "fluid/dlg_about.hpp"

extern "C" 
{
    extern char _binary_res_LICENSE_txt_start;
    extern char _binary_res_LICENSE_txt_end;
    extern char _binary_res_LICENSE_txt_size;
}

void dlg_about::create_ex()
{
    // Set the window icon
    utils::set_window_icon(m_window); 

    m_buf = new Fl_Text_Buffer();
    
    std::string v(std::string("Version: ") + std::string(FLORB_PROGSTR));
    std::string l(&_binary_res_LICENSE_txt_start, (size_t)&_binary_res_LICENSE_txt_size);

    m_buf->append(v.c_str());
    m_buf->append("\n\n");
    m_buf->append(l.c_str());

    m_display->buffer(m_buf);
}

void dlg_about::destroy_ex()
{
    if (m_window)
        delete(m_window);

    if (m_buf)
        delete(m_buf);
}

void dlg_about::show_ex()
{
    // Show the window
    m_window->show();

    for (;;) {
        Fl_Widget *o = Fl::readqueue();
        if (!o) Fl::wait();
        else if (o == m_btn_ok)     {break;}
        else if (o == m_window)     {break;}
    }

    m_window->hide();
}

