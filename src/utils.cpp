
#include <stdlib.h>
#include <string.h>

#include "panglos/debug.h"

    /*
     *  Utilities
     */

#include "i2s.h"
#include "rtp.h"
#include "utils.h"

void audio_copy(AudioSource *src, RTP_Engine *rtp, bool *)
{
    int idx = 0;

    //while (!*dead)
    while(true)
    {
        // blocking read on source
        src->read(rtp->rx_buff(idx), rtp->rx_bytes(idx));
        // send the RTP data
        const size_t samples = rtp->rx_bytes(idx) / (2 * sizeof(int16_t));
        rtp->send(idx, samples);
        idx = idx ? 0 : 1;
    }
}

    /*
     *
     */

#include <math.h>

#include "sockets.h"

void make_1kHz(RTP_Engine *rtp, int gain)
{
    PO_DEBUG("");

    size_t n_samples = rtp->num_samples * 2;
    uint16_t *samples = new uint16_t[n_samples];
    for (uint16_t i = 0; i < rtp->num_samples; i++)
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

    for (int idx = 0; idx < 2; idx++)
    {
        memcpy(rtp->rx_buff(idx), samples, rtp->rx_bytes(idx));
    }
    delete[] samples;
}

#include "panglos/time.h"

size_t Test_1kHz_Source::read(void *dest, size_t bytes)
{
    // Blocking read
    UNUSED(dest);
    panglos::Time::msleep(10);
    return bytes;
}

Test_1kHz_Source::Test_1kHz_Source(RTP_Engine *rtp)
{
    make_1kHz(rtp, 0x1000);
}

void run_audio_copy(void *arg)
{
    ASSERT(arg);
    AudioCopy *ac = (AudioCopy *) arg;
    audio_copy(ac->src, ac->dst, ac->dead);
}

//  FIN
