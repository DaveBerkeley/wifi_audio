

#include <stdint.h>
#include <string.h>

#include <driver/gpio.h>

#include "panglos/debug.h"

#include "panglos/device.h"
#include "panglos/object.h"
#include "panglos/time.h"
#include "panglos/storage.h"

#include "panglos/esp32/gpio.h"
#include "panglos/esp32/rmt_strip.h"

#include "panglos/app/devices.h"
#include "panglos/app/event.h"
#include "panglos/app/cli_server.h"

#include "board.h"

#include "esp32/init.h"
#include "esp32/rmt_led.h"

#include "status.h"

using namespace panglos;

#if defined(ESP32_LORA)

#if 0
	-D CONFIG_CS_GPIO=18
	-D CONFIG_RST_GPIO=14
	-D CONFIG_MOSI_GPIO=27
	-D CONFIG_SCK_GPIO=5
	-D CONFIG_MISO_GPIO=19
#endif

#define SCK GPIO_NUM_17
#define WS GPIO_NUM_16
#define SD GPIO_NUM_4

#define RGB_GPIO GPIO_NUM_2

static const struct LedsDef leds_def = {
    .pin=RGB_GPIO, 
    .n=1,
    .type=RmtLedStrip::Type::WS2812B
};

    /*
     *
     */

static const GPIO_DEF led_def = { GPIO_NUM_15, ESP_GPIO::OP, true };

    /*
     *
     */

static Device _board_devs[] = {
    DEV_GPIO("led", 0, & led_def),
    Device("rgb", 0, leds_init, (void*) & leds_def, Device::F_CAN_FAIL),
    Device(0, 0, 0, 0, 0),
};

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

#endif  //  ESP32_LORA

//  FIN
