
    /*
     *
     */

class RTP_Engine;

struct AudioCopy
{
    AudioSource *src;
    RTP_Engine *dst;
    bool *dead;
};

void run_audio_copy(void *arg);

//  FIN
