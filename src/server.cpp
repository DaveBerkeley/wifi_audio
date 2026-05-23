
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

static void server(void *arg)
{
    PO_DEBUG("");
    ASSERT(arg);

    struct ServerDesc *info = (struct ServerDesc *) arg;

    PO_DEBUG("RTP_Engine(%d,%d)", info->rtp_ports[1], info->rtp_ports[1]);
    RTP_Engine *rtp = new RTP_Engine(info->rtp_ports[1], info->rtp_ports[1], 2);
    Objects::objects->add("rtp", rtp);

    AudioSource *src = (I2S*) Objects::objects->get("i2s");
    if (!src)
    {
        static Test_1kHz_Source test(rtp);
        src = & test;
    }

    Thread *thread = Thread::create("rtp");
    static struct AudioCopy ac = { .src = src, .dst = rtp };
    thread->start(run_audio_copy, & ac);

    // blocking call to run server
    SID sid;
    char port[18];
    snprintf(port, sizeof(port), "%d", info->rtsp_port);
    PO_DEBUG("RTSP(%s:%s)", info->ip, port);
    rtsp_server(info->ip, port, rtp, & sid);
}

void run_server(struct ServerDesc *info)
{
    PO_DEBUG("");
    Thread *thread = Thread::create("rtsp");
    thread->start(server, info);
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
        cli_print(cli, "client %p: %d%s", client, client->get_num_packets(), cli->eol);
    }
}

void add_rtp_commands(CLI *cli)
{
    static CliCommand cmd = { "rtp", cmd_rtp, "list rtp clients", 0, 0, 0 };
    cli_insert(cli, & cli->head, & cmd);
}

//  FIN
