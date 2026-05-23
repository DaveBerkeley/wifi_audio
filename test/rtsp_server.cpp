
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
    RTP_Engine rtp(6000, 6001, 2);

    Test_1kHz_Source source(& rtp);

    Thread *thread = Thread::create("audio");
    static bool dead = false;
    struct AudioCopy ac = { .src = & source, .dst = & rtp, .dead = & dead };
    thread->start(run_audio_copy, & ac);

    // blocking call to run server
    SID sid;
    const char *ip = "0.0.0.0"; // all interfaces
    rtsp_server(ip, "8554", & rtp, & sid);

    thread->join();
    delete thread;
}

//  FIN
