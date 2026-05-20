
#include <string.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"

#include "rtp.h"

    /*
     *
     */

RTP_Client::RTP_Client(panglos::Socket *s)
:   socket(s),
    num_packets(0),
    next(0)
{
}

RTP_Client::~RTP_Client()
{
}

int RTP_Client::send(const uint8_t *data, size_t len)
{
    ASSERT(socket);
    num_packets += 1;
    return socket->send(data, len);
}

    /*
     *
     */

RTP_Engine::RTP_Engine(int rtp, int rtcp)
:   rtp_port(rtp),
    rtcp_port(rtcp),
    playing(RTP_Client::get_next),
    mutex(0),
    packet_seq(1),
    timestamp(0)
{
    mutex = panglos::Mutex::create();

    // initialise the RTP packet buffer
    // TODO : assumes zero CSRC and Extension blocks in header
    for (int idx = 0; idx < 2; idx++)
    {
        size_t packet_size = sizeof(RTP_Header) + (sizeof(uint16_t) * 2 * num_samples);
        uint8_t *data = new uint8_t[packet_size];
        memset(data, 0, packet_size);
        RTP_Header *packet = (RTP_Header*) data;

        packet->set_version(2);
        packet->set_payload(get_payload_type());
        packets[idx] = packet;
    }
}

RTP_Engine::~RTP_Engine()
{
    delete mutex;
    for (int idx = 0; idx < 2; idx++)
    {
        delete[] packets[idx];
    }
}

int16_t *RTP_Engine::rx_buff(int idx)
{
    ASSERT(idx < 2);
    return packets[idx]->get_audio();
}

size_t RTP_Engine::rx_bytes(int)
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
    // we map this to 16-bit PCM in the RSTP/SDP DESCRIBE Response
    return 96;
}

void RTP_Engine::play(RTP_Client *client)
{
    PO_DEBUG("%p", client);
    if (client)
        playing.push(client, mutex);
}

void RTP_Engine::remove(RTP_Client *client)
{
    PO_DEBUG("%p", client);
    if (client)
        playing.remove(client, mutex);
}

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
    client->send(b->data, b->size);
    return 0;
}

int RTP_Engine::send(int idx, size_t samples)
{
    if (samples > num_samples) return false;
    if (samples <= 0) return false;
    // increment the seq id
    ASSERT(idx < 2);
    RTP_Header *packet = packets[idx];
    packet->set_seq(packet_seq++);
    packet->set_timestamp(timestamp);
    timestamp += (int32_t) samples;
    // send the packet to all the clients in PLAY state
    size_t packet_size = sizeof(RTP_Header) + (sizeof(uint16_t) * 2 * samples);
    struct Buffer params = { .data = (uint8_t*) packet, .size = packet_size };
    playing.visit(send_packet, & params, mutex);
    return true;
}

    /*
     *  Stats. interface
     */

struct Counter
{
    int idx;    // position in the playing list
    int match;  // idx to match (-1 for none)
    RTP_Client *client; // matched client (at idx)
};

static int find_client(RTP_Client *client, void *arg)
{
    ASSERT(client);
    ASSERT(arg);
    struct Counter *counter = (struct Counter *) arg;
    if (counter->idx == counter->match)
    {
        // found the Client
        counter->client = client;
        return 1; // terminate search
    }
    counter->idx += 1;
    return 0;
}

int RTP_Engine::get_num_clients()
{
    struct Counter counter = {
        .idx = 0,
        .match = -1,
    };
    playing.visit(find_client, & counter, mutex);
    return counter.idx;
}

RTP_Client* RTP_Engine::get_client(int idx)
{
    struct Counter counter = {
        .idx = 0,
        .match = idx,
    };
    playing.visit(find_client, & counter, mutex);
    return counter.client;
}

//  FIN
