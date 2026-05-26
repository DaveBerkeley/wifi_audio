
#include <stdint.h>
#include <string.h>

#include "panglos/debug.h"

#include "panglos/esp32/gpio.h"
#include "panglos/device.h"
#include "panglos/object.h"
#include "panglos/storage.h"

#include "panglos/app/devices.h"
#include "panglos/app/event.h"
#include "panglos/app/cli_server.h"

using namespace panglos;

#include "i2s.h"
#include "server.h"
#include "esp32/init.h"
#include "audio_codec.h"

static struct ServerDesc info = {
    .ip = "0.0.0.0",
    .rtsp_port = 554,
    .rtp_ports = { 6000, 6001, },
};

static struct PcmConfig pcm_config = {
    .bits = 16,
    .chans = 2,
    .freq = 48000,
};

static struct OpusConfig opus_config = {
    .bit_rate = 96000,
    .complexity = 8,
    .packet_rate = 10, // ms
};

static bool rtp_init(void *arg, Event *, Event::Queue *)
{
    PO_DEBUG("");
    ASSERT(arg);

    I2S *i2s = (I2S*) Objects::objects->get("i2s");
    ASSERT(i2s);

    AudioCodec *codec = (AudioCodec*) arg;
    ASSERT(codec);
    info.codec = codec;
    run_server(& info);

    CLI *cli = (CLI*) Objects::objects->get("cli");
    ASSERT(cli);
    add_rtp_commands(cli);
 
    return false; // INIT handlers must return false so multiple handlers can be run
}

void board_init(int sck, int ws, int sd)
{
    PO_DEBUG("");

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

    ASSERT(codec);
    I2S *i2s = I2S::create(sck, ws, sd, 48000, !codec->network_order());
    ASSERT(i2s);
    Objects::objects->add("i2s", i2s);

    if (Objects::objects->get("net"))
    {
        EventHandler::add_handler(Event::INIT, net_cli_init, 0);
    }

    EventHandler::add_handler(Event::INIT, rtp_init, codec);    
}

//  FIN
