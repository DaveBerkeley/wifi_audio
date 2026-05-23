
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

//#define GPIO_TEST

#define SCK GPIO_NUM_12
#define WS GPIO_NUM_13
#define SD GPIO_NUM_14

#if defined(GPIO_TEST)
#define INPUT ESP_GPIO::IP | ESP_GPIO::PU
static const GPIO_DEF sck_def = { SCK, INPUT };
static const GPIO_DEF ws_def = { WS, INPUT };
static const GPIO_DEF sd_def = { SD, INPUT };
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
    board_init(SCK, WS, SD);
#endif

#if 0
    // Dammit, I've used the RGB LED for +5V I2C, so probably blown it up.
    int num = 1;
    RmtLedStrip *leds = RmtLedStrip::create(num, 24, RmtLedStrip::SK68XX);
    bool ok = leds->init(0, GPIO_NUM_38);
    ASSERT(ok);
    leds->set(0, 0x40, 0x40, 0x40);
#endif
}

Device *board_devs = _board_devs;

#endif  //  ESP32_S3_DKC1

//  FIN
