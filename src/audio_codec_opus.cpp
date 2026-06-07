
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "opus.h"

#include "panglos/debug.h"

#include "audio_codec.h"

    /*
     *
     */

static void *scratch = 0;

extern "C" {

    void *opus_alloc_scratch(size_t size)
    {
        PO_DEBUG("size=%d", (int) size);
        ASSERT(!scratch);
        scratch = malloc(size);
        ASSERT(scratch);
        return scratch;
    }
};

    /*
     *
     */

class OpusCodec : public AudioCodec
{
    OpusEncoder *encoder;
    OpusDecoder *decoder;
    char *sdp_fmt;
    uint32_t samples_per_block;
    struct OpusConfig config;
    const int channels = 2;

    bool make_encoder()
    {
        PO_DEBUG("bit_rate=%d complexity=%d packet_rate=%d ms",
            config.bit_rate,
            config.complexity,
            config.packet_rate);

        int err = OPUS_OK;
        encoder = opus_encoder_create(48000, channels, OPUS_APPLICATION_AUDIO, & err);
        if (err != OPUS_OK)
        {
            error(err, "opus_encoder_create()");
            return false;
        }

        err = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(config.bit_rate));
        if (err != OPUS_OK)
        {
            error(err, "opus_encoder_ctl(OPUS_SET_BITRATE)");
            return false;
        }
        err = opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(config.complexity));
        if (err != OPUS_OK)
        {
            error(err, "opus_encoder_ctl(OPUS_SET_COMPLEXITY)");
            return false;
        }

        return true;
    }

    bool make_decoder()
    {
        PO_DEBUG("bit_rate=%d complexity=%d packet_rate=%d ms",
            config.bit_rate,
            config.complexity,
            config.packet_rate);

        int size = opus_decoder_get_size(channels);

        decoder = (OpusDecoder*)malloc(size_t(size));
        ASSERT(decoder);

        int err = opus_decoder_init(decoder, 48000, channels);
        if (err != OPUS_OK)
        {
            error(err, "opus_encoder_create()");
            return false;
        }
        
        return true;
    }

    void make_sdp_fmt()
    {
        const char *fmt = 
            "k=clear: Opus Encoder\r\n"
            "v=0\r\n" // protocol version
            "o=- %%d %%d IN IP4 %%s\r\n" // session_id, session_version, ip_addr
            "s=Audio Stream\r\n"
            "c=IN IP4 0.0.0.0\r\n" // connection data - fixed during SETUP phase
            "t=0 0\r\n" // timing 
            "m=audio 0 RTP/AVP %d\r\n" // media announcement : 96 = dynamic payload
            "a=rtpmap:%d opus/48000/2\r\n" // map the payload type
            // ????????
            // "a=fmtp:%d maxplaybackrate=16000; stereo=1\r\n"
            // "a=ptime=20\r\n"
            // "a=maxptime=120\r\n"
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
        if (!sdp_fmt)
        {
            make_sdp_fmt();
        }
        ASSERT(sdp_fmt);
        return sdp_fmt;
    }

    virtual const char *name() override
    {
        return "Opus";
    }

    virtual uint8_t get_payload_type() override
    {
        return 97;
    }

    virtual bool network_order() override
    {
        return false; // requires I2S data to be swapped
    }

    virtual size_t samples_per_packet() override
    {
        ASSERT(samples_per_block);
        return samples_per_block;
    }

    virtual size_t num_chans() override
    {
        return 2;
    }

    virtual size_t data_size() override
    {
        return sizeof(int16_t);
    }

    virtual size_t max_payload_size() override
    {
        return 1275; // largest single frame Opus packet
    }

    void error(int code, const char *text)
    {
        PO_ERROR("%s %s", text, opus_strerror(code));
    }

    virtual size_t encode(const int16_t *src, size_t samples, uint8_t *dst, size_t obytes) override
    {
        ASSERT(encoder);
        opus_int32 err = opus_encode(encoder, src, (int) samples, dst, (int) obytes);
        if (err <= 0)
        {
            error(err, "opus_encoder()");
            return 0;
        }

        return size_t(err);
    }

    virtual size_t decode(const uint8_t *src, size_t ibytes, int16_t *dst, size_t obytes) override
    {
        ASSERT(decoder);
        const int samples = opus_decode(decoder, src, (opus_int32) ibytes, dst, (int) obytes, 0);
        return size_t(samples);
    }

public:
    OpusCodec(struct OpusConfig *_config)
    :   encoder(0),
        decoder(0),
        sdp_fmt(0),
        samples_per_block(0),
        config(*_config)
    {
        ASSERT(config.encode || config.decode); // must request encoder and/or decoder
        if (config.encode)
        {
            make_encoder();
        }
        if (config.decode)
        {
            make_decoder();
        }

        // Calcuate the number of audio sample periods in each packet
        // at 48kHz sample rate, 48 sample per ms
        ASSERT(config.packet_rate);
        samples_per_block = config.packet_rate * 48;
    }

    ~OpusCodec()
    {
        PO_DEBUG("");
        if(encoder) opus_encoder_destroy(encoder);
        free(decoder);
        free(sdp_fmt);
        free(scratch);
        scratch = 0;
    }
};

AudioCodec *AudioCodec::create(struct OpusConfig *config)
{
    ASSERT(config);
    ASSERT((config->bit_rate >= 6000) && (config->bit_rate <= 510000));
    ASSERT(config->complexity < 11);
    return new OpusCodec(config);
}

//  FIN
