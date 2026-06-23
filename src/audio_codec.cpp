
#include <stdint.h>
#include <string.h>

#include "panglos/debug.h"

#include "panglos/storage.h"

using namespace panglos;

#include "audio_codec.h"

//#define MAKE_OPUS
//#define MAKE_CODEC2
//#define MAKE_PCM

#if defined(MAKE_OPUS)

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

#endif  //  MAKE_OPUS

    /*
     *
     */

#if defined(MAKE_CODEC2)

#include "codec2/src/codec2.h"

static bool validate_mode(int32_t v, const char *name)
{
    const int32_t set[] = { 
        CODEC2_MODE_3200 ,
        CODEC2_MODE_2400,
        CODEC2_MODE_1600,
        CODEC2_MODE_1400,
        CODEC2_MODE_1300,
        CODEC2_MODE_1200,
        CODEC2_MODE_700C,
    };    
    return Storage::validate_set(v, name, set, sizeof(set)/sizeof(set[0]));
}

static AudioCodec *make_codec2()
{
    PO_DEBUG("");

    Storage db("codec2");

    int32_t mode = 1;
    int32_t fs = 8000; // Must be 8kHz

    struct Storage::IntParam params[] = {
        {   "mode", & mode, validate_mode },
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

#if defined(MAKE_PCM)

static AudioCodec *make_pcm()
{
    PO_DEBUG("");

    Storage db("pcm");

    int32_t bits = 16;
    int32_t chans = 2;
    int32_t freq = 48000; // Hz

    struct Storage::IntParam params[] = {
        {   "mode",    & bits,  },
        {   "chans",   & chans, },
        {   "freq",    & freq,  },
        { 0, 0 },
    };
 
    db.get_params(params);
    db.show_params(params);

    static struct PcmConfig pcm_config = {
        .bits = 16,
        .chans = 2,
        .freq = 48000,
    };

    PO_INFO("Creating PCM codec");
    return AudioCodec::create(& pcm_config);
}

#endif

    /*
     *
     */

AudioCodec *AudioCodec::make_codec()
{
    Storage db("app");

    char name[64];
    size_t size = sizeof(name);
    if (db.get("codec", name, & size))
    {

#if defined(MAKE_OPUS)
        if (!strcmp("opus", name))
        {
            return make_opus();
        }
#endif

#if defined(MAKE_CODEC2)
        if (!strcmp("codec2", name))
        {
            return make_codec2();
        }
#endif

#if defined(MAKE_PCM)
        if (!strcmp("pcm", name))
        {
            return make_pcm();
        }
#endif
    }

    return 0;
}

//  FIN
