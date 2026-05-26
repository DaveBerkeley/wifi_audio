
    /*
     *
     */

class RTP_Engine;

class Test_1kHz_Source : public AudioSource
{
    virtual size_t read(void *dest, size_t bytes) override;
public:
    Test_1kHz_Source(RTP_Engine *rtp);
};

void audio_copy(AudioSource *src, RTP_Engine *dst, bool *dead);

    /*
     *
     */

struct AudioCopy
{
    AudioSource *src;
    RTP_Engine *dst;
};

void run_audio_copy(void *arg);

//  FIN
