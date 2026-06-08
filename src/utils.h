
    /*
     *
     */

class AudioSource;
class RTP_Engine;
class Allocator;
class Reader;

namespace panglos {
    class Thread;
    class Mutex;
    class Queue;
}

    /*
     *
     */

struct AudioCopy
{
    AudioSource *src;
    RTP_Engine *rtp;
    Reader *reader;
    bool *dead;
    bool poll_reader;
};

class Reader
{
public:
    struct Block
    {
        int8_t *data;
        size_t bytes;
    };

    virtual ~Reader() {}

    virtual bool acquire() = 0;
    virtual bool read(struct Block *block) = 0;
    virtual void run() = 0;

    static Reader *create(struct AudioCopy*);
    static void run_reader(void*);
};

void audio_copy(AudioCopy *ac);

void run_audio_copy(void *arg);

//  FIN
