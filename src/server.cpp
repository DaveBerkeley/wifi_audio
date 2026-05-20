
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

struct Params {
    RTP_Engine *rtp;
    bool dead;
};

static void audio_gen(void *arg)
{
    PO_DEBUG("");
    ASSERT(arg);
    struct Params *params = (struct Params *) arg;

    make_1kHz(params->rtp, 0x1000);

    PO_DEBUG("");
    while (!params->dead)
    {
        Time::msleep(10);
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

    /*
     *
     */

struct ServerDesc
{
    const char *ip;
    int rtsp_port;
    int rtp_ports[2];
};

static void server(void *arg)
{
    PO_DEBUG("");
    ASSERT(arg);

    struct ServerDesc *info = (struct ServerDesc *) arg;

    PO_DEBUG("RTP_Engine(%d,%d)", info->rtp_ports[1], info->rtp_ports[1]);
    RTP_Engine *rtp = new RTP_Engine(info->rtp_ports[1], info->rtp_ports[1]);
    Objects::objects->add("rtp", rtp);

    Thread *thread = Thread::create("rtp");
    struct Params params { .rtp = rtp, .dead = false };
    thread->start(audio_gen, & params);

    // blocking call to run server
    SID sid;
    char port[18];
    snprintf(port, sizeof(port), "%d", info->rtsp_port);
    PO_DEBUG("RTSP(%s:%s)", info->ip, port);
    rtsp_server(info->ip, port, rtp, & sid);
}

void run_server()
{
    PO_DEBUG("");
    static struct ServerDesc info = {
        .ip = "0.0.0.0",
        .rtsp_port = 554,
        .rtp_ports = { 6000, 6001, },
    };
    Thread *thread = Thread::create("rtsp");
    thread->start(server, & info);
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
