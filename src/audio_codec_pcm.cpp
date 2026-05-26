
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "panglos/debug.h"

#include "audio_codec.h"

class PcmCodec : public AudioCodec
{
    char *sdp_fmt;

    void make_sdp_fmt(uint32_t bits, uint32_t chans, uint32_t freq)
    {
        const char *fmt = 
            "k=clear: PCM Encoder\r\n"
            "v=0\r\n" // protocol version
            "o=- %%d %%d IN IP4 %%s\r\n" // session_id, session_version, ip_addr
            "s=Audio Stream\r\n"
            "c=IN IP4 0.0.0.0\r\n" // connection data - fixed during SETUP phase
            "t=0 0\r\n" // timing 
            "m=audio 0 RTP/AVP %d\r\n" // media announcement : 96 = dynamic payload
            "a=rtpmap:%d L%d/%d/%d\r\n" // map the payload type
            ;

        // Note the 'L' in "L16/48000/2" means 'large' ie big-endian, not little!

        const size_t size = 1024;
        char *buff = new char[size];

        snprintf(buff, size, fmt, 
            get_payload_type(), get_payload_type(),
            bits, freq, chans
            );
        sdp_fmt = strdup(buff);
        delete[] buff;
    }

    virtual const char *get_sdp_fmt() override
    {
        ASSERT(sdp_fmt);
        return sdp_fmt;
    }

    virtual const char *name() override { return "PCM"; }

    virtual int get_payload_type() override
    {
        return 96;
    }

    virtual bool network_order() override
    {
        return true; // keep I2S order
    }
public:
    PcmCodec(uint32_t bits, uint32_t chans, uint32_t freq)
    :   sdp_fmt(0)
    {
        make_sdp_fmt(bits, chans, freq);
    }

    ~PcmCodec()
    {
        free(sdp_fmt);
    }
};

    /*
     *
     */

AudioCodec *AudioCodec::create(struct PcmConfig *config)
{
    ASSERT(config);
    return new PcmCodec(config->bits, config->chans, config->freq);
}

//  FIN
