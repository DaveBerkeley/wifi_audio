
#pragma once

#include <stdint.h>

class AudioCodec
{
public:
    enum codec_t
    {
        CODEC_PCM = 1,
        CODEC_OPUS = 1,
    };

    virtual ~AudioCodec() { }

    virtual const char *get_sdp_fmt() = 0;
    virtual const char *name() = 0;
    virtual int get_payload_type() = 0;
};

AudioCodec *create_pcm(uint32_t bits, uint32_t chans, uint32_t freq);

//  FIN
