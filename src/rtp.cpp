
#include "panglos/debug.h"
#include "panglos/mutex.h"

#include "rtp.h"

    /*
     *
     */

RTP_Client::RTP_Client(panglos::Socket *s)
:   socket(s)
{
}

RTP_Client::~RTP_Client()
{
}

    /*
     *
     */

RTP_Engine::RTP_Engine(int rtp, int rtcp)
:   rtp_port(rtp),
    rtcp_port(rtcp),
    packet(0),
    playing(RTP_Client::get_next),
    mutex(0)
{
    mutex = panglos::Mutex::create();
}

RTP_Engine::~RTP_Engine()
{
    delete mutex;
}

void RTP_Engine::get_server_ports(int *a, int *b)
{
    if (a) *a = rtp_port;
    if (b) *b = rtcp_port;
}

int RTP_Engine::get_payload_type()
{
    return 96;
}

void RTP_Engine::play(RTP_Client *client)
{
    PO_DEBUG("%p", client);
    playing.push(client, mutex);
}

void RTP_Engine::remove(RTP_Client *client)
{
    PO_DEBUG("%p", client);
    if (client)
        playing.remove(client, mutex);
}

//void RTP_Engine::pause(RTP_Client *);

//  FIN
