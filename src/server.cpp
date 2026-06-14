
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
#define CPU_AUX  1 // -1 for "no affinity"

void _server(void *arg)
{
    PO_DEBUG("");

    struct ServerDesc *info = (struct ServerDesc *) arg;
    ASSERT(info);
    ASSERT(info->codec);
    PO_DEBUG("codec=%s", info->codec->name());
    ASSERT(info->audio_source);

    const int num_buffers = 4;
    RTP_Engine *rtp = new RTP_Engine(info->codec, info->rtp_ports[0], info->rtp_ports[1], num_buffers, info->allocator);
    Objects::objects->add("rtp", rtp);

    bool dead = false;
    struct AudioCopy ac = {
        .src = info->audio_source, 
        .rtp = rtp,
        .reader = 0,
        .dead = & dead,
    };

    Thread *i2s_reader = Thread::create("i2s", 0, Thread::High);
    Reader *reader = Reader::create(& ac);
    ac.reader = reader;
    i2s_reader->start(Reader::run_reader, & ac, CPU_AUX);

    Thread *encoder_thread = Thread::create("encode", 5000, Thread::High);
    encoder_thread->start(run_audio_copy, & ac, CPU_CORE);

    // blocking call to run server
    SID sid;
    char port[18];
    snprintf(port, sizeof(port), "%d", info->rtsp_port);
    PO_DEBUG("RTSP(%s:%s)", info->ip, port);
    rtsp_server(info->ip, port, rtp, info->cb, info->codec, & sid);

    PO_INFO("RTSP Server shut down");

    // kill the RTP thread too
    PO_INFO("Kill RTP Thread");
    dead = true;
    encoder_thread->join();
    delete encoder_thread;
    i2s_reader->join();
    delete i2s_reader;
    delete reader;
    delete rtp;
    PO_INFO("RTSP/RTP shut down");

    delete info->codec;
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
        cli_print(cli, "client=%p: packets=%d tx errors=%d%s", 
            client, 
            client->get_num_packets(), 
            client->get_num_errors(), 
            cli->eol);
    }

    I2S *i2s = (I2S*) Objects::objects->get("i2s");
    if (i2s)
    {
        cli_print(cli, "i2s rx bytes=%d errors=%d%s", 
                i2s->rx_bytes(), i2s->rx_errors(), cli->eol);
    }
}

void add_rtp_commands(CLI *cli)
{
    static CliCommand cmd = { "rtp", cmd_rtp, "list rtp clients", 0, 0, 0 };
    cli_append(cli, & cmd);
}

//  FIN
