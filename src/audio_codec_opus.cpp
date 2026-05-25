
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "opus.h"

#include "panglos/debug.h"

#include "audio_codec.h"

class OpusCodec : public AudioCodec
{
    OpusEncoder *encoder;
    char *sdp_fmt;

    void make_sdp_fmt()
    {
        const char *fmt = 
            "v=0\r\n" // protocol version
            "o=- %%d %%d IN IP4 %%s\r\n" // session_id, session_version, ip_addr
            "s=Audio Stream\r\n"
            "c=IN IP4 0.0.0.0\r\n" // connection data - fixed during SETUP phase
            "t=0 0\r\n" // timing 
            "m=audio 0 RTP/AVP %d\r\n" // media announcement : 96 = dynamic payload
            "a=rtpmap:%d L16/48000/2\r\n" // map the payload type
            ;

        // Note the 'L' in "L16/48000/2" means 'large' ie big-endian, not little!

        const size_t size = 1024;
        char *buff = new char[size];

        snprintf(buff, size, fmt, get_payload_type(), get_payload_type());
        sdp_fmt = strdup(buff);
        delete[] buff;
    }

    virtual const char *get_sdp_fmt() override
    {
        ASSERT(sdp_fmt);
        return sdp_fmt;
    }

    virtual const char *name() override
    {
        return "Opus";
    }

    virtual int get_payload_type() override
    {
        return 97;
    }

    virtual bool network_order() override
    {
        return false; // requires I2S data to be swapped
    }

public:
    OpusCodec()
    :   encoder(0),
        sdp_fmt(0)
    {
#if 0
        int error;
        encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, & error);
        if (error != OPUS_OK)
        {
            PO_ERROR("opus_encoder_create() %s", opus_strerror(error));
            return;
        }

        opus_encoder_ctl(encoder, OPUS_SET_BITRATE(96000));
        opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(8));
#endif
    }

    ~OpusCodec()
    {
        //opus_encoder_destroy(encoder);
        free(sdp_fmt);
    }
};

AudioCodec *AudioCodec::create(struct OpusConfig *config)
{
    ASSERT(config);
    ASSERT(config->freq == 48000);
    ASSERT(config->chans == 2);
    return new OpusCodec;
}

//  FIN
