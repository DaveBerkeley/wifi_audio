
#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/thread.h"
#include "panglos/time.h"

using namespace panglos;

#include "sockets.h"

#include "rtsp_server.h"
#include "rtp.h"

struct Params {
    RTP_Engine *rtp;
    bool dead;
};

static void audio_gen(void *arg)
{
    ASSERT(arg);
    struct Params *params = (struct Params *) arg;

    make_1kHz(params->rtp, 0x1000);

    while (!params->dead)
    {
        Time::msleep(10);
        //PO_DEBUG("");
        params->rtp->send(params->rtp->num_samples);
    }
}

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

TEST(RtspServer, Test)
{
    RTP_Engine rtp(6000, 6001);

    Thread *thread = Thread::create("audio");
    struct Params params { .rtp = & rtp, .dead = false };
    thread->start(audio_gen, & params);

    // blocking call to run server
    SID sid;
    const char *ip = "0.0.0.0"; // all interfaces
    rtsp_server(ip, "8554", & rtp, & sid);

    params.dead = true;
    thread->join();
    delete thread;
}

//  FIN
