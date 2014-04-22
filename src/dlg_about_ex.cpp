#include <string>
#include "utils.hpp"
#include "version.hpp"
#include "fluid/dlg_about.hpp"

void dlg_about::create_ex()
{
    // Set the window icon
    utils::set_window_icon(m_window); 

    m_buf = new Fl_Text_Buffer();

    std::string v(std::string("Version: ") + std::string(FLORB_PROGSTR));

    m_buf->append(v.c_str());
    m_buf->append("\n\n");

    m_buf->append("Copyright (c) 2014 Bjoern Rehm <bjoern@shugaa.de>");
    m_buf->append("\n\n");
    m_buf->append("Distributed under the terms of the MIT license as stated below:");
    m_buf->append("\n\n");
    m_buf->append("Permission is hereby granted, free of charge, to any person obtaining a copy\n");
    m_buf->append("of this software and associated documentation files (the \"Software\"), to deal\n");
    m_buf->append("in the Software without restriction, including without limitation the rights\n");
    m_buf->append("to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n");
    m_buf->append("copies of the Software, and to permit persons to whom the Software is\n");
    m_buf->append("furnished to do so, subject to the following conditions:");
    m_buf->append("\n\n");
    m_buf->append("The above copyright notice and this permission notice shall be included in\n");
    m_buf->append("all copies or substantial portions of the Software.");
    m_buf->append("\n\n");
    m_buf->append("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n");
    m_buf->append("IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY\n");
    m_buf->append("FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n");
    m_buf->append("AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n");
    m_buf->append("LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n");
    m_buf->append("OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n");
    m_buf->append("THE SOFTWARE.");

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

