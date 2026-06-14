
#include <stdlib.h>
#include <string.h>

#include "panglos/debug.h"
#include "panglos/time.h"
#include "panglos/queue.h"
#include "panglos/mutex.h"

using namespace panglos;

#include "i2s.h"
#include "rtp.h"
#include "utils.h"
#include "audio_codec.h"

    /*
     *  Utilities
     */

class _Reader : public Reader
{
    AudioSource *src;
    bool *dead;
    size_t size;
    size_t max_read;
    Allocator *allocator;
    int num_blocks;
    int idx;
    panglos::Queue *queue;
    panglos::Mutex *mutex;

    int8_t **data;
public:
    _Reader(AudioSource *s, bool *_dead, size_t sz, size_t max_rd, Allocator *alloc, int _num_blocks)
    :   src(s),
        dead(_dead),
        size(sz),
        max_read(max_rd),
        allocator(alloc),
        num_blocks(_num_blocks),
        idx(0),
        queue(0),
        mutex(0),
        data(0)
    {
        ASSERT(dead);
        ASSERT(alloc);

        //PO_DEBUG("s=%d", (int)(num_blocks * sizeof(int8_t*)));
        data = (int8_t**) allocator->malloc(size_t(num_blocks) * sizeof(int8_t*));

        for (int i = 0; i < num_blocks; i++)
        {
            //PO_DEBUG("s=%d", (int)(size));
            data[i] = (int8_t *) allocator->malloc(size);
            ASSERT(data[i]);
        }

        mutex = Mutex::create();
        queue = Queue::create(sizeof(struct Block), num_blocks, mutex);
    }

    ~_Reader()
    {
        for (int i = 0; i < num_blocks; i++)
        {
            allocator->free(data[i]);
        }
        allocator->free(data);
        delete queue;
        delete mutex;
    }

    virtual bool acquire() override
    {
        int8_t *buff = data[idx];
        idx = (idx + 1) % num_blocks;
        // The I2S read buffer may be smaller than the codec's 
        // required input buffer, so do multiple reads
        // until we have a full buffer.
        size_t total = 0;
        int idx = 0;
        while (total < size)
        {
            size_t todo = size - total;
            size_t block = (todo > max_read) ? max_read : todo;
            size_t rd = src->read(& buff[total], block, idx++);
            total += rd;
            if (!rd) break;
        }

        struct Block block;
        block.data = buff;
        block.bytes = total;
        queue->put((Queue::Message*) & block);
        return total;
    }

    virtual void run() override
    {
        while (!*dead)
        {
            if (!acquire())
                break;
        }
        PO_DEBUG("done");
    }

    virtual bool read(struct Block *block) override
    {
        return queue->get((Queue::Message*) block, -1);
    }
};

    /*
     *
     */

Reader *Reader::create(AudioCopy *ac)
{
    ASSERT(ac);
    PO_DEBUG("");
    AudioSource *src = ac->src;
    RTP_Engine *rtp = ac->rtp;
    ASSERT(src);
    ASSERT(rtp);

    const size_t max_read = src->max_read_bytes();
    // allocate a read block for the source data
    size_t isize = rtp->rx_bytes();

    return new _Reader(src, ac->dead, isize, max_read, rtp->get_allocator(), 2);
}

void Reader::run_reader(void *arg)
{
    PO_DEBUG("");
    ASSERT(arg);
    AudioCopy *ac = (AudioCopy *) arg;
    ASSERT(ac->reader);
    ac->reader->run();
}

    /*
     *
     */

void audio_copy(AudioCopy *ac)
{
    ASSERT(ac);
    AudioSource *src = ac->src;
    RTP_Engine *rtp = ac->rtp;
    Reader *reader = ac->reader;
    bool *dead = ac->dead;
    ASSERT(src);
    ASSERT(rtp);
    ASSERT(reader);
    ASSERT(dead);
    AudioCodec *codec = rtp->get_codec();
    ASSERT(codec);

    const size_t samples = codec->samples_per_packet();

    int loops = 0;

    while (!*dead)
    {
        // blocking read on source
        if (ac->poll_reader)
            if (!reader->acquire())
                break;
        Reader::Block audio;
        if (!reader->read(& audio))
            break;

        loops += 1;
        if (!rtp->has_clients())
        {
            // no need to call the codec or send any RTP packets
            continue;
        }

        // get the next packet buffer
        RTP_Engine::Block *block = rtp->get_free();
        if (!block)
        {
            PO_ERROR("no free blocks!");
            Time::msleep(2);
            continue;
        }
 
        // compress the data
        uint8_t *odata = block->packet->get_audio();
        size_t sz = codec->encode((int16_t*) audio.data, samples, odata, rtp->rx_bytes());
        // send the RTP data
        rtp->send(block, sz, samples);
    }

    PO_DEBUG("done %d loops", loops);
}

    /*
     *
     */

void run_audio_copy(void *arg)
{
    ASSERT(arg);
    AudioCopy ac = *(AudioCopy *) arg;
    audio_copy(& ac);
}

//  FIN
