
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
    packet(0),
    playing(RTP_Client::get_next),
    mutex(0),
    packet_seq(1),
    timestamp(0)
{
    mutex = panglos::Mutex::create();

    // initialise the RTP packet buffer
    // TODO : assumes zero CSRC and Extension blocks in header
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

int16_t *RTP_Engine::rx_buff()
{
    return packet->get_audio();
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

    /*
     *  Utilities
     */

#include <math.h>

#include "sockets.h"

void make_1kHz(RTP_Engine *rtp, int gain)
{
    PO_DEBUG("");

    int n_samples = rtp->num_samples * 2;
    uint16_t *samples = new uint16_t[n_samples];
    for (uint16_t i = 0; i < rtp->num_samples; i++)
    {
        // generate 1kHz sine wave. 48kHz sample rate, 48 samples per cycle
        const int iphase = i % 48;
        double phase = (iphase * M_PI * 2) / 48;
        const double sine = sin(phase);
        int16_t sample = (int16_t)(gain * sine);
        sample = ntohs(sample);
        samples[i*2] = sample;
        samples[(i*2)+1] = sample;
    }

    memcpy(rtp->rx_buff(), samples, rtp->rx_size());
    delete[] samples;
}

//  FIN
