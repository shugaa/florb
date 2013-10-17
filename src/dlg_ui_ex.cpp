#include <Fl/Fl_File_Chooser.H>
#include "gpxlayer.hpp"
#include "mapctrl.hpp"
#include "fluid/orb_dlg_ui.hpp"

void orb_dlg_ui::cb_btn_loadtrack_ex(Fl_Widget *widget)
{
    // Create a file chooser instance
    Fl_File_Chooser fc("/", "*.gpx", Fl_File_Chooser::SINGLE, "Open GPX file");
    fc.preview(0);
    fc.show();

    // Wait for user action
    while(fc.shown())
        Fl::wait();

    // Do nothing on cancel
    if (fc.value() == NULL)
        return;

    // Try to create a new GPX layer from the file
    layer *l = new gpxlayer(std::string(fc.value()));

    // Display the new layer
    m_mapctrl->push_layer(l);
}
