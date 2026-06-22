
#include <stdint.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"

#include "cli/src/cli.h"

#include "panglos/debug.h"

#include "panglos/esp32/gpio.h"
#include "panglos/device.h"
#include "panglos/object.h"
#include "panglos/storage.h"
#include "panglos/network.h"

#include "panglos/app/devices.h"
#include "panglos/app/event.h"

using namespace panglos;

#include "../i2s.h"
#include "server.h"
#include "esp32/init.h"
#include "esp32/i2s.h"
#include "audio_codec.h"
#include "rtp.h"
#include "rtsp.h"

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

struct SystemConfig
{
    AudioCodec *codec;
    RTSP_Status *cb;
};

static bool rtp_init(void *arg, Event *, Event::Queue *)
{
    PO_DEBUG("");
    ASSERT(arg);

    struct SystemConfig *config = (struct SystemConfig *) arg;

    I2S *i2s = (I2S*) Objects::objects->get("i2s");
    ASSERT(i2s);

    AudioCodec *codec = config->codec;
    ASSERT(codec);
    info.codec = codec;
    info.allocator = new HeapAllocator;
    info.audio_source = i2s;
    info.cb = config->cb;
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
     *  Validate Opus config
     */

static bool validate_complexity(int32_t v, const char *name)
{

    return Storage::validate_range(v, name, 0, 10);
}

static bool validate_bit_rate(int32_t v, const char *name)
{

    return Storage::validate_range(v, name, 5000, 510000);
}

static bool validate_fs(int32_t v, const char *name)
{

    return Storage::validate_range(v, name, 8000, 48000);
}

static bool validate_chans(int32_t v, const char *name)
{

    return Storage::validate_range(v, name, 1, 2);
}

static bool validate_app(int32_t v, const char *name)
{
    const int32_t set[] = { OpusConfig::OP_AUDIO, OpusConfig::OP_VOIP };
    return Storage::validate_set(v, name, set, sizeof(set)/sizeof(set[0]));
}

static bool validate_rate(int32_t v, const char *name)
{
    const int32_t set[] = { /*3,*/ 5, 10, 20, 40, 60, /*120,*/ };
    return Storage::validate_set(v, name, set, sizeof(set)/sizeof(set[0]));
}

    /*
     *
     */

static AudioCodec *make_opus()
{
    PO_DEBUG("");

    Storage db("opus");

    int32_t bit_rate = 96000;
    int32_t complexity = 5;
    int32_t packet_rate = 60; // ms
    int32_t fs = 48000; // Hz
    int32_t chans = 2;
    int32_t app = OpusConfig::OP_AUDIO;

    struct Storage::IntParam params[] = {
        {   "bit_rate",    & bit_rate,    validate_bit_rate },
        {   "complexity",  & complexity,  validate_complexity },
        {   "packet_rate", & packet_rate, validate_rate },
        {   "fs",          & fs,          validate_fs },
        {   "chans",       & chans,       validate_chans },
        {   "app",         & app,         validate_app },
        { 0, 0 },
    };
 
    db.get_params(params);
    db.show_params(params);

    struct OpusConfig opus_config = {
        .bit_rate    = (uint32_t) bit_rate,
        .complexity  = (uint32_t) complexity,
        .packet_rate = (uint32_t) packet_rate, // ms
        .fs = (uint32_t) fs,
        .chans = (uint32_t) chans,
        .app = (OpusConfig::Application) app,
        .encode = true,
    };

    return AudioCodec::create(& opus_config);
}

    /*
     *
     */

//#define MAKE_CODEC2

#if defined(MAKE_CODEC2)

static bool validate_bit_mode(int32_t v, const char *name)
{
    const int32_t set[] = { 0, 1, 2, 3, 4, 5, 8 };
    return Storage::validate_set(v, name, set, sizeof(set)/sizeof(set[0]));
}

static AudioCodec *make_codec2()
{
    PO_DEBUG("");

    Storage db("opus");

    int32_t mode = 1;
    int32_t fs = 48000; // Hz

    struct Storage::IntParam params[] = {
        {   "mode",    & mode,    validate_bit_mode },
        //{   "fs",          & fs,          validate_fs },
        { 0, 0 },
    };
 
    db.get_params(params);
    db.show_params(params);

    struct Codec2Config config = {
        .mode = (uint32_t) mode,
        .fs   = (uint32_t) fs,
    };

    return AudioCodec::create(& config);
}
#endif  //  MAKE_CODEC2

    /*
     *
     */

static AudioCodec *make_codec()
{
    Storage db("app");

    char name[64];
    size_t size = sizeof(name);
    if (db.get("codec", name, & size))
    {
        if (!strcmp("opus", name))
        {
            return make_opus();
        }
#if defined(MAKE_CODEC2)
        if (!strcmp("codec2", name))
        {
            return make_codec2();
        }
#endif
        if (strcmp("pcm", name))
        {
            PO_ERROR("Unknown codec '%s'", name);
            return 0;
        }
    }

    PO_INFO("Creating PCM codec");
    return AudioCodec::create(& pcm_config);
}

    /*
     *
     */

static ESP32_I2S::Config i2s_config;

static I2S *make_i2s(AudioCodec *codec, gpio_num_t sck, gpio_num_t ws, gpio_num_t sd)
{
    PO_DEBUG("");

    ASSERT(codec);
    Storage db("i2s");

    int32_t bits = 16;
    int32_t slot_bits = 16;
    int32_t freq = 48000;
    int32_t chans = 2;;
    int32_t primary = 0; // generate I2S clock outputs

    struct Storage::IntParam params[] = {
        {   "bits", & bits },
        {   "slot_bits", & slot_bits },
        {   "freq", & freq },
        {   "chans", & chans },
        {   "primary", & primary },
        { 0, 0 },
    };
 
    db.get_params(params);
    db.show_params(params);

    ESP32_I2S::Config config = {
        .pins = {
            .mck = I2S_GPIO_UNUSED,
            .sck = sck,
            .ws = ws,
            .din = sd,
            .dout = I2S_GPIO_UNUSED,
        },
        .freq = (uint32_t) freq,
        .bits = (uint32_t) bits,
        .slot_bits = (uint32_t) slot_bits,
        .chans = (uint32_t) chans,
        .byte_swap = codec->network_order(),
        .primary = (bool) primary,
    };

    // keep a copy of the last config, for the CLI test stuff
    i2s_config = config; 

    return ESP32_I2S::create(& config);
}

    /*
     *
     */

//#define CLI_TEST

#if defined(CLI_TEST)

#include "panglos/thread.h"
#include "panglos/storage.h"

static void test_fn(struct CLI *cli, struct CliCommand *)
{
    if (Thread::get_by_name("i2s"))
    {
        cli_print(cli, "Error: 'i2s' thread still running%s", cli->eol);
        return;
    }

    I2S *i2s = (I2S*) Objects::objects->get("i2s");

    if (i2s)
    {
        cli_print(cli, "delete old i2s%s", cli->eol);
        delete i2s;
        Objects::objects->remove("i2s");
        i2s = 0;
    }

    struct Arg
    {
        const char *name;
        const int32_t *set;
        size_t sz;
        int value;
    };

    const int32_t bits[] = { 8, 16, 24, 32 };
    //const int32_t freq[] = { 48000, 96000 };
    const int32_t chans[] = { 1, 2 };

    struct Arg args[] = {
        {   "bits", bits, sizeof(bits)/sizeof(bits[0]), },
        {   "slot", bits, sizeof(bits)/sizeof(bits[0]) },
        {   "freq", }, // freq, sizeof(freq)/sizeof(freq[0]) },
        {   "chans", chans, sizeof(chans)/sizeof(chans[0]) },
        {   0 },
    };

    int idx = 0;
    for (struct Arg *arg = args; arg->name; arg++)
    {
        const char *s = cli_get_arg(cli, idx++);
        if (!s)
        {
            cli_print(cli, "error: expected '%s' value%s", arg->name, cli->eol);
            return;
        }
        if (!cli_parse_int(s, & arg->value, 10))
        {
            cli_print(cli, "errror parsing '%s'=%s%s", arg->name, s, cli->eol);
            return;
        }

        if (arg->set)
        {
            if (!Storage::validate_set(arg->value, arg->name, arg->set, arg->sz))
            {
                cli_print(cli, "invalid value '%s'=%d for '%s'%s", arg->name, (int) arg->value, s, cli->eol);
                return;
            }
        }

        cli_print(cli, "%s=%d%s", arg->name, arg->value, cli->eol);
    }

    // Get the last settings
    ESP32_I2S::Config config = i2s_config;
    config.bits      = int32_t(args[0].value);
    config.slot_bits = int32_t(args[1].value);
    config.freq      = int32_t(args[2].value);
    config.chans     = int32_t(args[3].value);

    i2s = ESP32_I2S::create(& config);
    Objects::objects->add("i2s", i2s);
    
    cli_print(cli, "created new I2S device%s", cli->eol);
}

static CliCommand test_cmd = { "test", test_fn, "help!", 0, 0, 0 };

static bool test_cli_init(void *arg, Event *, Event::Queue *)
{
    PO_DEBUG("");
    UNUSED(arg);

    CLI *cli = (CLI*) Objects::objects->get("cli");
    ASSERT(cli);
    cli_append(cli, & test_cmd);

    return false; // INIT handlers must return false so multiple handlers can be run
}

#endif  //  CLI_TEST

    /*
     *
     */

void board_init(RTSP_Status *cb, gpio_num_t sck, gpio_num_t ws, gpio_num_t sd)
{
    PO_DEBUG("");

    if (!network_running())
    {
        PO_ERROR("No Network Unable to run RTSP/RTP servers.");
        return;
    }

    AudioCodec *codec = make_codec();
    if (!codec)
    {
        PO_ERROR("No Codec!");
        return;
    }

    I2S *i2s = make_i2s(codec, sck, ws, sd);
    if (!i2s)
    {
        PO_ERROR("No I2S input device!");
        return;
    }

    Objects::objects->add("i2s", i2s);

    static SystemConfig config = {
        .codec = codec,
        .cb = cb,
    };

    EventHandler::add_handler(Event::INIT, rtp_init, & config);
#if defined(CLI_TEST)
    EventHandler::add_handler(Event::INIT, test_cli_init, & config);
#endif
}

//  FIN
