
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//  https://github.com/drowe67/codec2

#include "codec2/src/codec2.h"

#include "panglos/debug.h"

#include "audio_codec.h"

//using namespace panglos;

    /*
     *
     */

static LUT mode_lut[] = {
    {   "3200", CODEC2_MODE_3200 },
    {   "2400", CODEC2_MODE_2400 },
    {   "1600", CODEC2_MODE_1600 },
    {   "1400", CODEC2_MODE_1400 },
    {   "1300", CODEC2_MODE_1300 },
    {   "1200", CODEC2_MODE_1200 },
    {   "700C", CODEC2_MODE_700C },
    {   0   },
};

class Codec2 : public AudioCodec
{
    struct CODEC2 *codec;
    const char *sdp;
    int mode;
    int fs;
public:
    Codec2(int _mode, int _fs)
    :   codec(0),
        sdp(0),
        mode(_mode),
        fs(_fs)
    {
        PO_DEBUG("mode=%s", lut(mode_lut, mode));
        codec = codec2_create(mode);
        ASSERT(codec);
    }

    ~Codec2()
    {
        if (codec) codec2_destroy(codec);
        free((char*)sdp);
    }

    const char *make_sdp()
    {
        const char *fmt = 
            "k=clear: Codec2 Encoder\r\n"
            "m=audio 0 RTP/AVP %d\r\n"
            "c=IN IP4 0.0.0.0\r\n"
            "a=rtpmap:%d codec2/%d\r\n"
            "a=fmtp:%d mode=%s\r\n"
        ;

        const size_t size = 1024;
        char *buff = new char[size];

        snprintf(buff, size, fmt, 
                get_payload_type(),
                get_payload_type(),
                fs,
                get_payload_type(),
                lut(mode_lut, mode));

        sdp = strdup(buff);
        delete[] buff;
        return sdp;
    }

    virtual const char *get_sdp_fmt() override
    {
        if (!sdp) 
        {
            sdp = make_sdp();
        }
        return sdp;
    }

    virtual const char *name() override
    {
        return "codec2";
    }

    virtual uint8_t get_payload_type() override
    {
        // RTP payload type
        return 98;
    }

    virtual bool network_order() override
    {
        return false;
    }

    virtual size_t samples_per_packet() override
    {
        ASSERT(codec);
        return codec2_samples_per_frame(codec);
    }

    virtual size_t num_chans() override
    {
        return 1;
    }

    virtual size_t sample_size() override
    {
        return sizeof(int16_t);
    }

    virtual size_t max_payload_size() override
    {
        return 128; // ??????????
    }

    virtual size_t encode(const int16_t *src, size_t samples, uint8_t *dst, size_t obytes) override
    {
        UNUSED(samples);
        UNUSED(obytes);
        ASSERT(sizeof(int16_t) == sizeof(short int));
        codec2_encode(codec, dst, (short int*) src);
        return codec2_bytes_per_frame(codec);
    }

    virtual size_t decode(const uint8_t *src, size_t ibytes, int16_t *dst, size_t obytes) override
    {
        ASSERT(0);
        UNUSED(src);
        UNUSED(ibytes);
        UNUSED(dst);
        UNUSED(obytes);
//void codec2_decode(struct CODEC2 *codec2_state, short speech_out[], const unsigned char bytes[]);        
        return 0;
    }
};

AudioCodec *AudioCodec::create(struct Codec2Config *config)
{
    ASSERT(config);
    return new Codec2(config->mode, config->fs);
}

//  FIN
