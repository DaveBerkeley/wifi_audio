
#include <math.h>

#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/thread.h"
#include "panglos/time.h"
#include "panglos/object.h"

using namespace panglos;

#include "sockets.h"

#include "rtsp_server.h"
#include "rtp.h"
#include "i2s.h"
#include "server.h"
#include "audio_codec.h"

class TestSource : public AudioSource
{
    int period; // ms
    uint16_t *samples;

    void init(int gain)
    {
        PO_DEBUG("");

        size_t n_samples = period * 48;
        samples = new uint16_t[n_samples*2];
        //memset(samples, 0, sizeof(uint16_t) * n_samples * 2);
        for (uint16_t i = 0; i < n_samples; i++)
        {
            // generate 1kHz sine wave. 48kHz sample rate, 48 samples per cycle
            const int iphase = i % 48;
            double phase = (iphase * M_PI * 2) / 48;
            const double sine = sin(phase);
            int16_t sample = (int16_t)(gain * sine);
            sample = ntohs(sample);
            samples[i*2] = sample;
            samples[(i*2)+1] = sample;
        }
    }

public:
    TestSource(int period_ms)
    :   period(period_ms),
        samples(0)
    {
        init(0x1000);
    }

    ~TestSource()
    {
        delete[] samples;
    }

    virtual size_t read(void *dest, size_t bytes) override
    {
        memcpy(dest, samples, bytes);
        Time::msleep(period);
        return bytes;
    }
};

    /*
     *
     */

class SID : public SidGenerator 
{
public:
    virtual uint32_t generate() override
    {
        static uint32_t sid = 1111;
        return sid++;
    }
};

#if 0
static struct PcmConfig codec_config = {
    .bits = 16,
    .chans = 2,
    .freq = 48000,
};
#else
static struct OpusConfig codec_config = {
    .bit_rate = 96000,
    .complexity = 8,
    .packet_rate = 20, // ms
};
#endif

TEST(RtspServer, Test)
{
    Objects::objects = Objects::create();
    AudioCodec *codec = AudioCodec::create(& codec_config);

    TestSource source(20);
    Objects::objects->add("i2s", & source);

    struct ServerDesc info = {
        .ip = "0.0.0.0", 
        .rtsp_port = 8554, 
        .rtp_ports = { 600, 6001 },
        .codec = codec,
    };

    _server(& info);

    delete codec;
    delete Objects::objects;
    Objects::objects = 0;
}

//  FIN
