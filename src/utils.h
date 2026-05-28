
    /*
     *
     */

class RTP_Engine;

class xTest_1kHz_Source : public AudioSource
{
    virtual size_t read(void *dest, size_t bytes, int idx) override;
public:
    xTest_1kHz_Source(RTP_Engine *rtp);
};

void audio_copy(AudioSource *src, RTP_Engine *dst, bool *dead);

    /*
     *
     */

struct AudioCopy
{
    AudioSource *src;
    RTP_Engine *dst;
    bool *dead;
};

void run_audio_copy(void *arg);

//  FIN
