
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
#define RGB_GPIO GPIO_NUM_38
#endif
#if defined(ESP32_S3_DKC2)
#define RGB_GPIO GPIO_NUM_48


#endif

//#define SDCARD_MISO  GPIO_NUM_41
//#define SDCARD_MOSI  GPIO_NUM_40
//#define SDCARD_CLK   GPIO_NUM_39
//#define SDCARD_CS    GPIO_NUM_42

    /*
     *
     */

static Device _board_devs[] = {
    Device(0, 0, 0, 0, 0),
};

class Callback : public RTSP_Status
{
    RmtLedStrip *leds;
    Mutex *mutex;

    typedef RTSP_Session::State State;
    typedef struct LedStrip::RGB RGB;

    static bool to_rgb(State state, RGB *rgb)
    {
        ASSERT(rgb);
        switch (state)
        {
            case RTSP_Session::INIT  :  return false;
            case RTSP_Session::READY :  rgb->b = 0x40;   break;
            case RTSP_Session::PLAY  :  rgb->g = 0x40;   break;
            case RTSP_Session::DEAD  :  break; // all off
            default : ASSERT(0);
        }

        return true;
    }

    virtual void on_state(State state) override
    {
        PO_DEBUG("state=%d", state);
        RGB rgb = { 0 };
        if (to_rgb(state, & rgb))
        {
            Lock lock(mutex);
            leds->set_all(rgb.r, rgb.g, rgb.b);
            leds->send();
        }
    }

public:

    Callback(RmtLedStrip *_leds)
    :   leds(_leds),
        mutex(0)
    {
        ASSERT(leds);
        mutex = Mutex::create();
    }

    ~Callback()
    {
        delete mutex;
    }
};

    /*
     *
     */

void board_init()
{
#if defined(RGB_GPIO)
    RmtLedStrip *leds = RmtLedStrip::create(1);
    bool ok = leds->init(0, RGB_GPIO);
    ASSERT(ok);
    Objects::objects->add("rgb", leds);

    leds->set_all(0x8, 0x8, 0x8);
    leds->send();
#endif

    static Callback cb(leds);
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
