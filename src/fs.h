
#pragma once

struct SpiPins
{
    gpio_num_t cs;
    gpio_num_t mosi;
    gpio_num_t miso;
    gpio_num_t ck;
};

bool init_fs(struct SpiPins *);

//  FIN
