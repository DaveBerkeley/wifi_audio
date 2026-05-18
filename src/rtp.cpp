
#include "panglos/debug.h"

#include "rtp.h"

    /*
     *
     */

RTP_Client::RTP_Client()
{
}

RTP_Client::~RTP_Client()
{
}

void RTP_Client::set_client_ports(int a, int b)
{
    rtp_port = a;
    rtcp_port = b;
}

panglos::Socket *RTP_Client::socket()
{
    ASSERT(0);
    return 0;
}

    /*
     *
     */

RTP_Engine::RTP_Engine(int rtp, int rtcp)
:   rtp_port(rtp),
    rtcp_port(rtcp),
    packet(0),
    playing(RTP_Client::get_next)
{
}

void RTP_Engine::get_server_ports(int *a, int *b)
{
    if (a) *a = rtp_port;
    if (b) *b = rtcp_port;
}

//void RTP_Engine::play(RTP_Client *);
//void RTP_Engine::pause(RTP_Client *);

//  FIN
