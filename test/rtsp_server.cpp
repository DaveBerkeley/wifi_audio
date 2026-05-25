
#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/thread.h"
#include "panglos/time.h"

using namespace panglos;

#include "sockets.h"

#include "rtsp_server.h"
#include "rtp.h"
#include "i2s.h"
#include "utils.h"
#include "audio_codec.h"

    /*
     *
     */

class SID : public SidGenerator 
{
public:
    virtual uint32_t generate() override
    {
        static uint32_t sid = 1111;
        return sid++;
    }
};

static struct PcmConfig pcm_config = {
    .bits = 16,
    .chans = 2,
    .freq = 48000,
};

TEST(RtspServer, Test)
{
    AudioCodec *codec = AudioCodec::create(& pcm_config);
    RTP_Engine rtp(codec, 6000, 6001, 2);

    Test_1kHz_Source source(& rtp);

    Thread *thread = Thread::create("audio");
    static bool dead = false;
    struct AudioCopy ac = { .src = & source, .dst = & rtp, .dead = & dead };
    thread->start(run_audio_copy, & ac);

    // blocking call to run server
    SID sid;
    const char *ip = "0.0.0.0"; // all interfaces
    rtsp_server(ip, "8554", & rtp, codec, & sid);

    thread->join();
    delete thread;
    delete codec;
}

//  FIN
