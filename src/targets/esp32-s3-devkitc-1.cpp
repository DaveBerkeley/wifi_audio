
#include <stdint.h>
#include <string.h>

#include <driver/gpio.h>

#include "panglos/debug.h"

#include "panglos/device.h"
#include "panglos/object.h"

#include "panglos/esp32/gpio.h"

#include "panglos/app/devices.h"

#include "board.h"

#include "esp32/init.h"

using namespace panglos;

#if defined(ESP32_S3_DKC1)

#define SCK GPIO_NUM_13
#define WS GPIO_NUM_14
#define SD GPIO_NUM_12

//#define RGB GPIO_NUM_11
// Dammit, I've used the RGB LED for +5V I2C, so probably blown it up.
//#define RGB GPIO_NUM_38

#define DEBUG_PIN GPIO_NUM_10

static const GPIO_DEF debug_def = { DEBUG_PIN, ESP_GPIO::OP };

    /*
     *
     */

static Device _board_devs[] = {
    //DEV_GPIO("led", 0, & led_def),
    DEV_GPIO("dbg", 0, & debug_def),
    Device(0, 0, 0, 0, 0),
};

void board_init()
{
    board_init(SCK, WS, SD);
}

Device *board_devs = _board_devs;

#endif  //  ESP32_S3_DKC1

//  FIN
