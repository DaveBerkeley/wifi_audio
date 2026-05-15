
#include "panglos/debug.h"

#include "panglos/esp32/gpio.h"
#include "panglos/device.h"
#include "panglos/object.h"

#include "panglos/esp32/spi.h"
#include "panglos/app/devices.h"
#include "panglos/app/event.h"
#include "panglos/app/cli_server.h"

#include "board.h"

using namespace panglos;

#if defined(ESP32C3_XIAO)

#define PMOD_0   GPIO_NUM_6
#define PMOD_1   GPIO_NUM_5
#define PMOD_2   GPIO_NUM_4
#define PMOD_3   GPIO_NUM_3
#define PMOD_4   GPIO_NUM_7
#define PMOD_5   GPIO_NUM_10
#define PMOD_6   GPIO_NUM_20
#define PMOD_7   GPIO_NUM_21

// no LED on the xiao board!
//static const GPIO_DEF led_def = { GPIO_NUM_8, ESP_GPIO::OP, true };

    /*
     *
     */

static Device _board_devs[] = {
    //DEV_GPIO("led", 0, & led_def),
    //DEV_GPIO("spi_cs", 0, & spi_cs_def),
    Device(0, 0, 0, 0, 0),
};

void board_init()
{
    PO_DEBUG("");

    EventHandler::add_handler(Event::INIT, net_cli_init, 0);    
}

Device *board_devs = _board_devs;

#endif  //  ESP32C3_XIAO

//  FIN
