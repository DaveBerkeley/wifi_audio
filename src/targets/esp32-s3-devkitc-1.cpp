
#include <stdint.h>
#include <string.h>

#include <driver/gpio.h>

#include "panglos/debug.h"

#include "cli/src/cli.h"

#include "panglos/device.h"
#include "panglos/object.h"
#include "panglos/cli_net.h"
#include "panglos/event_queue.h"

#include "panglos/esp32/rmt_strip.h"

#include "panglos/esp32/gpio.h"

#include "panglos/app/event.h"
#include "panglos/app/devices.h"
#include "panglos/app/cli_server.h"

#include "board.h"

#include "esp32/init.h"

using namespace panglos;

#if defined(ESP32_S3_DKC1)

#define SCK GPIO_NUM_12
#define WS GPIO_NUM_13
#define SD GPIO_NUM_14

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

#if defined(RGB)
    int num = 2;
    RmtLedStrip *leds = RmtLedStrip::create(num, 24, RmtLedStrip::Type::WS2812B); // , RmtLedStrip::SK68XX);
    bool ok = leds->init(0, RGB);
    ASSERT(ok);
    leds->set(0, 0x40, 0x40, 0x40);
    leds->set(1, 0x40, 0, 0);
#endif
}

Device *board_devs = _board_devs;

#endif  //  ESP32_S3_DKC1

//  FIN
