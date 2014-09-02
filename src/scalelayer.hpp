#ifndef SCALELAYER_HPP
#define SCALELAYER_HPP

#include "layer.hpp"
#include "viewport.hpp"

class scalelayer : public florb::layer
{
    public:
        scalelayer();
        ~scalelayer();

        bool draw(const viewport &viewport, florb::canvas &os);
    private:
};

#endif // SCALELAYER_HPP

