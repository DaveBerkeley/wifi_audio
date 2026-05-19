
#include <string.h>

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
    mutex(0),
    packet_seq(1),
    timestamp(0)
{
    mutex = panglos::Mutex::create();

    // initialise the RTP packet buffer
    size_t packet_size = sizeof(RTP_Header) + (sizeof(uint16_t) * 2 * num_samples);
    uint8_t *data = new uint8_t[packet_size];
    memset(data, 0, packet_size);
    packet = (RTP_Header*) data;

    packet->set_version(2);
    packet->set_payload(get_payload_type());
    packet->set_seq(packet_seq++);
    packet->set_timestamp(timestamp);
}

RTP_Engine::~RTP_Engine()
{
    delete mutex;
    delete[] packet;
}

uint16_t *RTP_Engine::rx_buff()
{
    return & packet->audio[0];
}

size_t RTP_Engine::rx_size()
{
    return num_samples * 2 * sizeof(uint16_t);
}

void RTP_Engine::get_server_ports(int *a, int *b)
{
    if (a) *a = rtp_port;
    if (b) *b = rtcp_port;
}

uint8_t RTP_Engine::get_payload_type()
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

    /*
     *  RTP Packet transmit
     */

struct Buffer
{
    uint8_t *data;
    size_t size;
};

static int send_packet(RTP_Client *client, void *arg)
{
    ASSERT(client);
    ASSERT(arg);
    struct Buffer *b = (struct Buffer *) arg;
    panglos::Socket *socket = client->get_socket();
    ASSERT(socket);
    socket->send(b->data, b->size);
    return 0;
}

int RTP_Engine::send(int samples)
{
    if (samples > num_samples) return false;
    if (samples <= 0) return false;
    // increment the seq id
    packet->set_seq(packet_seq++);
    packet->set_timestamp(timestamp);
    timestamp += samples;
    // send the packet to all the clients in PLAY state
    size_t packet_size = sizeof(RTP_Header) + (sizeof(uint16_t) * 2 * samples);
    struct Buffer params = { .data = (uint8_t*) packet, .size = packet_size };
    playing.visit(send_packet, & params, mutex);
    return true;
}

//  FIN
