
#pragma once

struct LedsDef {
    uint32_t pin;
    int n;
    int type;
};

namespace panglos
{
    class Device;
};

bool leds_init(panglos::Device *dev, void *arg);

//  FIN
