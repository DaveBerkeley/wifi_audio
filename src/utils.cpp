
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

void audio_copy(AudioSource *src, RTP_Engine *rtp, bool *dead)
{
    ASSERT(src);
    ASSERT(rtp);
    AudioCodec *codec = rtp->get_codec();
    ASSERT(codec);

    // allocate a read block for the source data
    size_t ibuff_size = rtp->rx_bytes();
    Allocator *allocator = rtp->get_allocator();
    ASSERT(allocator);
    uint8_t *idata = (uint8_t *) allocator->malloc(ibuff_size);
    //memset(idata, 0, ibuff_size);
    ASSERT(idata);
    const size_t samples = codec->samples_per_packet();

    const size_t max_read = src->max_read_bytes();
    const size_t passes = (ibuff_size  + max_read - 1) / max_read;
    PO_DEBUG("max_read=%d passes=%d", (int) max_read, (int) passes);
    PO_DEBUG("block_size=%d samples=%d", (int) rtp->rx_bytes(), (int) samples);

    while (!*dead)
    {
        // blocking read on source
        // use multiple passes until the whole buffer has been read
        size_t total = 0;
        int idx = 0;
        while (total < ibuff_size)
        {
            size_t todo = ibuff_size - total;
            size_t block = (todo > max_read) ? max_read : todo;
            src->read(& idata[total], block, idx++);
            total += block;
            //PO_DEBUG("read bytes=%d total=%d", (int) block, (int) total);
        }

        if (!rtp->has_clients())
        {
            // no need to call the codec or send any RTP packets
            continue;
        }

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
        size_t sz = codec->process(idata, samples, odata, rtp->rx_bytes());
        // send the RTP data
        rtp->send(block, sz, samples);
    }

    PO_DEBUG("");
    allocator->free(idata);
}

    /*
     *
     */

void run_audio_copy(void *arg)
{
    ASSERT(arg);
    AudioCopy *ac = (AudioCopy *) arg;
    audio_copy(ac->src, ac->dst, ac->dead);
}

//  FIN
