#ifndef GPSDLAYER_HPP
#define GPSDLAYER_HPP

#include <layer.hpp>
#include "viewport.hpp"
#include "gpsdclient.hpp"

class gpsdlayer : public layer, gpsdclient_observer
{
    public:
        gpsdlayer();
        ~gpsdlayer();

        void gpsdclient_notify(void);
        static void gpsdclient_callback(void *data);
        void gpsdclient_process(void);

        void draw(const viewport &viewport,canvas &os);

    private:
        gpsdclient *m_gpsdclient;
};

#endif // GPSDLAYER_HPP

