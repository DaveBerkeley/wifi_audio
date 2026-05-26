
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
#include "audio_codec.h"

void audio_copy(AudioSource *src, RTP_Engine *rtp)
{
    ASSERT(src);
    ASSERT(rtp);
    AudioCodec *codec = rtp->get_codec();
    ASSERT(codec);

    // allocate a read block for the source data
    size_t ibuff_size = codec->samples_per_packet() * codec->num_chans() * codec->data_size();
    Allocator *allocator = rtp->get_allocator();
    ASSERT(allocator);
    uint8_t *idata = (uint8_t *) allocator->malloc(ibuff_size);
    ASSERT(idata);

    PO_DEBUG("block_size=%d", (int) rtp->rx_bytes());
    //while (!*dead)
    while (true)
    {
        // blocking read on source
        src->read(idata, ibuff_size);

        // get the next block RAM
        RTP_Engine::Block *block = rtp->get_free();
        if (!block)
        {
            //PO_ERROR("no free blocks!");
            panglos::Time::msleep(2);
            continue;
        }
        
        // compress the data
        uint8_t *odata = block->packet->get_audio();
        size_t sz = codec->process(idata, ibuff_size, odata, rtp->rx_bytes());
        // send the RTP data
        rtp->send(block, sz, codec->samples_per_packet());
    }

    PO_DEBUG("");
    allocator->free(idata);
}

    /*
     *
     */

#if 0

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

#endif

void run_audio_copy(void *arg)
{
    ASSERT(arg);
    AudioCopy *ac = (AudioCopy *) arg;
    audio_copy(ac->src, ac->dst);
}

//  FIN
