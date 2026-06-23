
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "panglos/debug.h"
#include "panglos/storage.h"

using namespace panglos;

#include "audio_codec.h"

#if defined(MAKE_PCM)

class PcmCodec : public AudioCodec
{
    char *sdp_fmt;
    uint32_t freq;

    void make_sdp_fmt(uint32_t bits, uint32_t chans)
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

    virtual const char *name() override { return "pcm"; }

    virtual uint8_t get_payload_type() override
    {
        return 96;
    }

    virtual bool network_order() override
    {
        return true; // keep I2S order
    }

    virtual size_t samples_per_packet() override
    {
        return 360; // gives a packet size < max UDP size
    }

    virtual size_t sample_size() override
    {
        return sizeof(int16_t);
    }

    virtual size_t num_chans() override
    {
        return 2;
    }

    virtual size_t max_payload_size() override
    {
        return samples_per_packet() * num_chans() * sizeof(int16_t);
    }

    virtual size_t encode(const int16_t *src, size_t samples, uint8_t *dst, size_t obytes) override
    {
        const size_t size = samples * num_chans() * sample_size();
        ASSERT(size <= obytes);
        memcpy(dst, src, size);
        return size;
    }

    virtual size_t decode(const uint8_t *, size_t , int16_t *, size_t ) override
    {
        ASSERT(0); // TODO
        return 0;
    }

    virtual size_t sample_rate() override
    {
        return freq;
    }

public:
    PcmCodec(uint32_t bits, uint32_t chans, uint32_t _freq)
    :   sdp_fmt(0),
        freq(_freq)
    {
        make_sdp_fmt(bits, chans);
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

    /*
     *  Build codec from config in Storage
     */

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

static AudioCodec::Register reg("pcm", make_pcm);

#endif  //  MAKE_PCM

//  FIN
