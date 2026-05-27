
#include <stddef.h>

#include "cli/src/cli.h"

#include "panglos/debug.h"
#include "panglos/thread.h"
#include "panglos/time.h"
#include "panglos/object.h"

using namespace panglos;

#include "sockets.h"

#include "rtsp_server.h"
#include "rtp.h"

#include "server.h"
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

    /*
     *
     */

#define CPU_CORE 0 // -1 for "no affinity"

void _server(void *arg)
{
    PO_DEBUG("");

    struct ServerDesc *info = (struct ServerDesc *) arg;
    ASSERT(info);
    ASSERT(info->codec);
    PO_DEBUG("codec=%s", info->codec->name());

    const int num_buffers = 4;
    PO_DEBUG("RTP_Engine(%d,%d,nbuffs=%d)", info->rtp_ports[1], info->rtp_ports[1], num_buffers);
    RTP_Engine *rtp = new RTP_Engine(info->codec, info->rtp_ports[1], info->rtp_ports[1], num_buffers, info->allocator);
    Objects::objects->add("rtp", rtp);

    AudioSource *src = (I2S*) Objects::objects->get("i2s");
    ASSERT(src);

    // The Opus encoder needs more stack
    Thread *thread = Thread::create("rtp", 8000);
    static struct AudioCopy ac = { .src = src, .dst = rtp };
    thread->start(run_audio_copy, & ac, CPU_CORE);

    // blocking call to run server
    SID sid;
    char port[18];
    snprintf(port, sizeof(port), "%d", info->rtsp_port);
    PO_DEBUG("RTSP(%s:%s)", info->ip, port);
    rtsp_server(info->ip, port, rtp, info->codec, & sid);

    ASSERT(0); // you can't leave
}

void run_server_thread(struct ServerDesc *info)
{
    PO_DEBUG("");
    Thread *thread = Thread::create("rtsp");
    thread->start(_server, info, CPU_CORE);
}

    /*
     *  CLI interface
     */

static void cmd_rtp(CLI *cli, CliCommand *)
{
    RTP_Engine *rtp = (RTP_Engine *) Objects::objects->get("rtp");
    ASSERT(rtp);

    // List clients / packet stats
    const int num = rtp->get_num_clients();
    cli_print(cli, "RTP %d Clients%s", num, cli->eol);
    for (int idx = 0; idx < num; idx++)
    {
        RTP_Client *client = rtp->get_client(idx);
        if (!client) continue;
        cli_print(cli, "client=%p: packets=%d errors=%d%s", 
            client, 
            client->get_num_packets(), 
            client->get_num_errors(), 
            cli->eol);
    }
}

void add_rtp_commands(CLI *cli)
{
    static CliCommand cmd = { "rtp", cmd_rtp, "list rtp clients", 0, 0, 0 };
    cli_insert(cli, & cli->head, & cmd);
}

//  FIN
