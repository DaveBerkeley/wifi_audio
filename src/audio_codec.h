
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
    uint32_t    bit_rate;      // eg 96000 bps
    uint32_t    complexity;    // 0 .. 10
    uint32_t    packet_rate;   // in ms { 3 (2.5), 5, 10, 20, 40, 60, 120 }
    bool        encode;
    bool        decode;
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
    virtual size_t data_size() = 0;
    virtual size_t max_payload_size() = 0;
    virtual size_t encode(const int16_t *src, size_t samples, uint8_t *dst, size_t obytes) = 0;
    virtual size_t decode(const uint8_t *src, size_t ibytes, int16_t *dst, size_t obytes) = 0;

    static AudioCodec *create(struct PcmConfig *);
    static AudioCodec *create(struct OpusConfig *);
};

//  FIN
