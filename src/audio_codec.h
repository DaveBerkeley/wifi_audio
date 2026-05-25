
#pragma once

#include <stdint.h>

class AudioCodec;

struct PcmConfig
{
    uint32_t bits;
    uint32_t chans;
    uint32_t freq;
};

struct OpusConfig
{
    // TODO 
    uint32_t bits;
    uint32_t chans;
    uint32_t freq;
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
    virtual int get_payload_type() = 0;
    virtual bool network_order() = 0;

    static AudioCodec *create(struct PcmConfig *);
    static AudioCodec *create(struct OpusConfig *);
};

//  FIN
