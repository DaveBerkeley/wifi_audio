
#include <stdlib.h>
#include <string.h>

#include "panglos/debug.h"
#include "panglos/time.h"

    /*
     *  Utilities
     */

#include "i2s.h"
#include "rtp.h"
#include "utils.h"

void audio_copy(AudioSource *src, RTP_Engine *rtp, bool *)
{
    //while (!*dead)
    while (true)
    {
        // get the next block RAM
        RTP_Engine::Block *block = rtp->get_free();
        if (!block)
        {
            //PO_ERROR("no free blocks!");
            panglos::Time::msleep(2);
            continue;
        }
        // blocking read on source
        uint8_t *data = block->packet->get_audio();
        src->read(data, rtp->rx_bytes());
        // send the RTP data
        block->samples = rtp->rx_bytes() / (2 * sizeof(int16_t));
        rtp->send(block);
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

    // Get all the available blocks
    RTP_Engine::Blocks blocks(RTP_Engine::Block::get_next);

    while (true)
    {
        RTP_Engine::Block *block = rtp->get_free();
        if (!block) break;
        blocks.push(block, 0);
    }

    while (true)
    {
        RTP_Engine::Block *block = blocks.pop(0);
        if (!block) break;
        memcpy(block->packet->get_audio(), samples, rtp->rx_bytes());
        rtp->put_free(block);        
    }
}

#include "panglos/time.h"

size_t Test_1kHz_Source::read(void *dest, size_t bytes)
{
    // Blocking read
    UNUSED(dest);
    panglos::Time::msleep(7);
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
