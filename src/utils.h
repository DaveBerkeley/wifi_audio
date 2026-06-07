
    /*
     *
     */

class AudioSource;
class RTP_Engine;

void audio_copy(AudioSource *src, RTP_Engine *rtp, bool *dead);

struct AudioCopy
{
    AudioSource *src;
    RTP_Engine *dst;
    bool *dead;
};

void run_audio_copy(void *arg);


//  FIN
