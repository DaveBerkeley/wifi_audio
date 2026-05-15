
    /*
     *
     */

#define DEV_GPIO(name, needs, def) Device(name, needs, gpio_init, (void*) def)

extern panglos::Device *board_devs;

void board_init();

    /*
     *
     */

#if defined (ESP32C3_XIAO)
#define BOARD_NAME "ESP32C3_XIAO"
#endif

//  FIN
