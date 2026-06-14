
#include <stdint.h>
#include <string.h>

#include <driver/gpio.h>

#include "panglos/debug.h"

#include "panglos/device.h"
#include "panglos/object.h"
#include "panglos/time.h"
#include "panglos/storage.h"

#include "panglos/esp32/rmt_strip.h"

#include "panglos/app/devices.h"
#include "panglos/app/event.h"
#include "panglos/app/cli_server.h"

#include "board.h"

#include "esp32/init.h"
#include "esp32/rmt_led.h"

#include "status.h"

using namespace panglos;

#if defined(ESP32_S3_DKC1) || defined(ESP32_S3_DKC2) || defined(ESP32_DEV)

#if defined(ESP32_S3_DKC1)
//  I2S Input
#define SCK GPIO_NUM_42
#define WS GPIO_NUM_2
#define SD GPIO_NUM_1

// but I've blown up the LED device on my dev board ...
#define RGB_GPIO GPIO_NUM_38
#endif

#if defined(ESP32_S3_DKC2)
//  I2S Input
#define SCK GPIO_NUM_42
#define WS GPIO_NUM_2
#define SD GPIO_NUM_1

#define RGB_GPIO GPIO_NUM_48
#endif

#if defined(ESP32_DEV)
//  I2S Input
#define SCK GPIO_NUM_12
#define WS GPIO_NUM_13
#define SD GPIO_NUM_14

#define RGB_GPIO GPIO_NUM_27
#endif

    /*
     *
     */

static const struct LedsDef leds_def = {
    .pin=RGB_GPIO, 
    .n=1,
    .type=RmtLedStrip::Type::WS2812B
};

    /*
     *
     */

static Device _board_devs[] = {
    Device("rgb", 0, leds_init, (void*) & leds_def, Device::F_CAN_FAIL),
    Device(0, 0, 0, 0, 0),
};

    /*
     *
     */

void board_init()
{
#if 0
    Storage db("app");
    db.erase("codec");
#endif

    LedStrip *leds = (LedStrip *) Objects::objects->get("rgb");
    ASSERT(leds);
    leds->set_all(0, 0, 0x10);
    leds->send();

    static Callback cb(leds);
    board_init(& cb, SCK, WS, SD);

    EventHandler::add_handler(Event::IDLE, Callback::on_idle, & cb);
    EventHandler::add_handler(Event::INIT, net_cli_init, 0);
}

Device *board_devs = _board_devs;

#endif  //  ESP32_S3_DKC1

//  FIN
