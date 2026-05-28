
#include <stdint.h>
#include <string.h>

#include "esp_heap_caps.h"

#include "panglos/debug.h"

#include "panglos/esp32/gpio.h"
#include "panglos/device.h"
#include "panglos/object.h"
#include "panglos/storage.h"
#include "panglos/network.h"

#include "panglos/app/devices.h"
#include "panglos/app/event.h"
#include "panglos/app/cli_server.h"

using namespace panglos;

#include "i2s.h"
#include "server.h"
#include "esp32/init.h"
#include "audio_codec.h"
#include "rtp.h"

class HeapAllocator : public Allocator
{
    virtual void* malloc(size_t bytes) override
    {
        const uint32_t caps = MALLOC_CAP_DMA | MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL;
        const size_t  alignment = 4;
        return heap_caps_aligned_alloc(alignment, bytes, caps);
    }

    virtual void free(void *data) override
    {
        heap_caps_free(data);
    }
};

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
    .packet_rate = 40, // ms
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
    info.allocator = new HeapAllocator;
    info.audio_source = i2s;
    run_server_thread(& info);

    CLI *cli = (CLI*) Objects::objects->get("cli");
    ASSERT(cli);
    add_rtp_commands(cli);
 
    return false; // INIT handlers must return false so multiple handlers can be run
}

    /*
     *
     */

static bool network_running()
{
    Network *net = (Network *) Objects::objects->get("net");
    if (!net)
    {
        PO_ERROR("no network");
        return false;
    }

    Interface *iface = net->get_interface();    
    if (!iface)
    {
        PO_ERROR("no interface running");
        return false;
    }

    return iface->is_connected(0);
}


    /*
     *
     */

void board_init(int sck, int ws, int sd)
{
    PO_DEBUG("");

    if (!network_running())
    {
        PO_ERROR("No Network. Unable to run RTSP/RTP servers.");
        return;
    }

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

    EventHandler::add_handler(Event::INIT, rtp_init, codec);    
    EventHandler::add_handler(Event::INIT, net_cli_init, 0);
}

//  FIN
