
#include <stdint.h>
#include <string.h>

#include <driver/gpio.h>

#include "panglos/debug.h"

#include "panglos/device.h"
#include "panglos/object.h"

#include "panglos/esp32/gpio.h"
#include "panglos/esp32/rmt_strip.h"

#include "panglos/app/devices.h"

#include "board.h"

#include "esp32/init.h"
#include "rtsp.h"

using namespace panglos;

#if defined(ESP32_S3_DKC1) || defined(ESP32_S3_DKC2)

#define SCK GPIO_NUM_42
#define WS GPIO_NUM_2
#define SD GPIO_NUM_1

#if defined(ESP32_S3_DKC1)
// but I've blown up the LED device on my dev board ...
#define RGB GPIO_NUM_38
#endif
#if defined(ESP32_S3_DKC2)
#define RGB GPIO_NUM_48


#endif

//#define SDCARD_MISO  GPIO_NUM_41
//#define SDCARD_MOSI  GPIO_NUM_40
//#define SDCARD_CLK   GPIO_NUM_39
//#define SDCARD_CS    GPIO_NUM_42

    /*
     *
     */

static Device _board_devs[] = {
    //DEV_GPIO("led", 0, & led_def),
    Device(0, 0, 0, 0, 0),
};

class Callback : public RTSP_Status
{
    virtual void on_state(RTSP_Session::State state) override
    {
        PO_DEBUG("state=%d", state);
    }
};

void board_init()
{
#if defined(RGB)
    RmtLedStrip *leds = RmtLedStrip::create(1);
    bool ok = leds->init(0, RGB);
    ASSERT(ok);
    Objects::objects->add("rgb", leds);

    leds->set_all(0x8, 0x8, 0x8);
    leds->send();
#endif

    static Callback cb;
    board_init(& cb, SCK, WS, SD);

#if 0
    struct SpiPins pins = {
        .cs = SDCARD_CS,
        .mosi = SDCARD_MOSI,
        .miso = SDCARD_MISO,
        .ck = SDCARD_CLK,
    };
    init_fs(& pins);
#endif
}

Device *board_devs = _board_devs;

#endif  //  ESP32_S3_DKC1

//  FIN
