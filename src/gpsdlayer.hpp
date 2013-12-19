#ifndef GPSDLAYER_HPP
#define GPSDLAYER_HPP

#include "layer.hpp"
#include "viewport.hpp"
#include "gpsdclient.hpp"

class gpsdlayer : public layer
{
    public:
        gpsdlayer();
        ~gpsdlayer();

        void draw(const viewport &viewport,canvas &os);

    private:
        struct gps_info {
            bool valid;
            point2d<double> pos;
            double track;
            int mode;
        }; 

        bool handle_evt_gpsupdate(const gpsdclient_update_event *e);

        gps_info m_gpsinfo;
        gpsdclient *m_gpsdclient;
};

#endif // GPSDLAYER_HPP

