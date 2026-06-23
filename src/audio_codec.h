
#pragma once

#include <stdint.h>

class AudioCodec;

struct PcmConfig
{
    uint32_t bits;  //  eg 16
    uint32_t chans; //  eg 2
    uint32_t freq;  //  eg 48000
};

struct OpusConfig
{
    enum Application {
        OP_AUDIO = 1,
        OP_VOIP = 2,
    };

    uint32_t    bit_rate;       // eg 96000 bps
    uint32_t    complexity;     // 0 .. 10
    uint32_t    packet_rate;    // in ms { 3 (2.5), 5, 10, 20, 40, 60, 120 }
    uint32_t    fs;             // Sample Frequency in Hz
    uint32_t    chans;          // number of channels
    enum Application app;       // AUDIO | VOIP
    bool        encode;
    bool        decode;
};

struct Codec2Config
{
    uint32_t    mode; // eg. CODEC2_MODE_1300 
    uint32_t    fs;   // only used in the SDP header
};

    /*
     *
     */

class AudioCodec
{
public:
    virtual ~AudioCodec() { }

    virtual const char *get_sdp_fmt() = 0;
    virtual const char *name() = 0;
    virtual uint8_t get_payload_type() = 0;
    virtual bool network_order() = 0;
    virtual size_t samples_per_packet() = 0;
    virtual size_t num_chans() = 0;
    virtual size_t sample_rate() = 0;
    virtual size_t sample_size() = 0;
    virtual size_t max_payload_size() = 0;
    virtual size_t encode(const int16_t *src, size_t samples, uint8_t *dst, size_t obytes) = 0;
    virtual size_t decode(const uint8_t *src, size_t ibytes, int16_t *dst, size_t obytes) = 0;

    static AudioCodec *create(struct PcmConfig *);
    static AudioCodec *create(struct OpusConfig *);
    static AudioCodec *create(struct Codec2Config *);

    static AudioCodec* make_codec(const char *name=0);

    // Allow codecs to be registered

    class Register
    {
    public:
        const char *name;
        AudioCodec* (*maker)();
        Register *next;

        Register(const char *_name, AudioCodec* (*fn)())
        :   name(_name),
            maker(fn),
            next(0)
        {
            next = codecs;
            codecs = this;
        }
    };

    static Register *codecs;
};

//  FIN
