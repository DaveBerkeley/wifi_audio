
#include "hal/gpio_types.h"

#include "panglos/debug.h"
#include "panglos/device.h"

#include "panglos/esp32/gpio.h"
#include "panglos/app/devices.h"

using namespace panglos;

#include "board.h"

#include "esp32/init.h"

#if defined(ESP32C3_XIAO)

// no LED on the xiao board!
//static const GPIO_DEF led_def = { GPIO_NUM_8, ESP_GPIO::OP, true };

static const GPIO_DEF sck_def = { GPIO_NUM_4, ESP_GPIO::IP };
static const GPIO_DEF ws_def = { GPIO_NUM_5, ESP_GPIO::IP };
static const GPIO_DEF sd_def = { GPIO_NUM_6, ESP_GPIO::IP };

    /*
     *
     */

static Device _board_devs[] = {
    //DEV_GPIO("led", 0, & led_def),
    DEV_GPIO("sck", 0, & sck_def),
    DEV_GPIO("ws", 0, & ws_def),
    DEV_GPIO("sd", 0, & sd_def),
    Device(0, 0, 0, 0, 0),
};

void board_init()
{
    //board_init(GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6);
}

Device *board_devs = _board_devs;

#endif  //  ESP32C3_XIAO

//  FIN
