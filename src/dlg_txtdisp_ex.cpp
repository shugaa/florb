#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include "utils.hpp"
#include "fluid/dlg_txtdisp.hpp"

void dlg_txtdisp::create_ex()
{
    // Set the window icon
    florb::utils::set_window_icon(m_window); 

    m_buf = new Fl_Text_Buffer();
    m_display->buffer(m_buf);
}

void dlg_txtdisp::clear_ex()
{
    if (m_buf == NULL)
        return;

    m_buf->text(NULL);
}

void dlg_txtdisp::append_ex(const std::string& s)
{
    if (m_buf == NULL)
        return;

    m_buf->append(s.c_str());
}

void dlg_txtdisp::append_ex(const std::vector<std::string>& v, const std::string& delimiter, bool translate)
{
    if (m_buf == NULL)
        return;

    std::vector<std::string>::const_iterator it;
    for (it=v.begin();it!=v.end();++it)
    {
        if (translate)
            m_buf->append(_((*it).c_str()));
        else
            m_buf->append((*it).c_str());

        if ((it+1) != v.end())
            m_buf->append(delimiter.c_str());
    }
}

void dlg_txtdisp::title_ex(const std::string& t)
{
    if (m_window == NULL)
        return;
    
    m_title = t;
    m_window->label(m_title.c_str());
}

void dlg_txtdisp::destroy_ex()
{
    if (m_window)
        delete(m_window);

    if (m_buf)
        delete(m_buf);
}

void dlg_txtdisp::show_ex()
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

