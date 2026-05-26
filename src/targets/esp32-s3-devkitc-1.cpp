
#include <stdint.h>
#include <string.h>

#include <driver/gpio.h>

#include "panglos/debug.h"

#include "panglos/device.h"
#include "panglos/object.h"
#include "panglos/storage.h"

#include "panglos/esp32/gpio.h"

#include "panglos/app/devices.h"

#include "board.h"

#include "esp32/init.h"
#include "audio_codec.h"

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

static struct PcmConfig pcm_config = {
    .bits = 16,
    .chans = 2,
    .freq = 48000,
};

static struct OpusConfig opus_config = {
    .bit_rate = 96000,
    .complexity = 8,
};

void board_init()
{
    AudioCodec *codec = 0;

    Storage db("app");

    char name[64];
    size_t size = sizeof(name);
    if (db.get("codec", name, & size))
    {
        if (!strcmp("opus", name))
        {
            codec = AudioCodec::create(& opus_config);
        }
        else if (!strcmp("pcm", name))
        {
            codec = AudioCodec::create(& pcm_config);
        }
    }

    if (!codec)
    {
        PO_INFO("Creating default codec");
        codec = AudioCodec::create(& pcm_config);
    }

    board_init(SCK, WS, SD, codec);

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
