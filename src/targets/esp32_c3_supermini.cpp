
#include "hal/gpio_types.h"

#include "panglos/debug.h"
#include "panglos/device.h"

#include "panglos/esp32/gpio.h"
#include "panglos/app/devices.h"

using namespace panglos;

#include "board.h"

#include "esp32/init.h"

#if defined(ESP32C3_SUPERMINI)

// no LED on the xiao board!
//static const GPIO_DEF led_def = { GPIO_NUM_8, ESP_GPIO::OP, true };

//#define GPIO_TEST

#if defined(GPIO_TEST)
#define INPUT ESP_GPIO::IP | ESP_GPIO::PU
static const GPIO_DEF sck_def = { GPIO_NUM_4, INPUT };
static const GPIO_DEF ws_def = { GPIO_NUM_3, INPUT };
static const GPIO_DEF sd_def = { GPIO_NUM_2, INPUT };
#endif

    /*
     *
     */

static Device _board_devs[] = {
    //DEV_GPIO("led", 0, & led_def),
#if defined(GPIO_TEST)
    DEV_GPIO("sck", 0, & sck_def),
    DEV_GPIO("ws", 0, & ws_def),
    DEV_GPIO("sd", 0, & sd_def),
#endif
    Device(0, 0, 0, 0, 0),
};

void board_init()
{
#if !defined(GPIO_TEST)
    board_init(GPIO_NUM_4, GPIO_NUM_3, GPIO_NUM_2);
#endif
}

Device *board_devs = _board_devs;

#endif  //  ESP32C3_XIAO

//  FIN
