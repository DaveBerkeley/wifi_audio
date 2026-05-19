
#include <stddef.h>

#include "panglos/debug.h"
#include "panglos/thread.h"
#include "panglos/time.h"

using namespace panglos;

#include "sockets.h"

#include "rtsp_server.h"
#include "rtp.h"

#include "server.h"

struct Params {
    RTP_Engine *rtp;
    bool dead;
};

static void audio_gen(void *arg)
{
    PO_DEBUG("");
    ASSERT(arg);
    struct Params *params = (struct Params *) arg;

    int n_samples = params->rtp->num_samples * 2;
    uint16_t *samples = new uint16_t[n_samples];
    for (uint16_t i = 0; i < params->rtp->num_samples; i++)
    {
        uint16_t sample = (i & 0x4) ? 0x4000 : -0x4000;
        sample = ntohs(sample);
        samples[i*2] = sample;
        samples[(i*2)+1] = sample;
    }

    memcpy(params->rtp->rx_buff(), samples, params->rtp->rx_size());

    PO_DEBUG("");
    while (!params->dead)
    {
        Time::msleep(40);
        //PO_DEBUG("");
        params->rtp->send(params->rtp->num_samples);
    }

    delete[] samples;
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

static void server(void *)
{
    PO_DEBUG("");
    RTP_Engine *rtp = new RTP_Engine(6000, 6001);

    Thread *thread = Thread::create("audio");
    struct Params params { .rtp = rtp, .dead = false };
    thread->start(audio_gen, & params);

    // blocking call to run server
    SID sid;
    const char *ip = "0.0.0.0"; // all interfaces
    rtsp_server(ip, "8554", rtp, & sid);
}

void run_server()
{
    PO_DEBUG("");
    Thread *thread = Thread::create("server");
    thread->start(server, 0);
}

//  FIN
