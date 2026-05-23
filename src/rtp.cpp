
#include <string.h>
#include <stdlib.h>

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
    delete socket;
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

class SystemAllocator : public Allocator
{
    virtual void *malloc(size_t sz)
    {
        return ::malloc(sz);
    }
    virtual void free(void *data)
    {
        ::free(data);
    }
};

static SystemAllocator allocator;

Allocator *Allocator::system()
{
    return & allocator;
}

    /*
     *
     */

RTP_Engine::RTP_Engine(int rtp, int rtcp, int num_buffers, Allocator *a)
:   rtp_port(rtp),
    rtcp_port(rtcp),
    allocator(a),
    playing(RTP_Client::get_next),
    mutex(0),
    packet_seq(1),
    timestamp(0),
    blocks(Block::get_next)
{
    allocator = allocator ? allocator : Allocator::system();
    mutex = panglos::Mutex::create();

    // TODO : assumes zero CSRC and Extension blocks in header
    for (int idx = 0; idx < num_buffers; idx++)
    {
        size_t packet_size = sizeof(RTP_Header) + (sizeof(uint16_t) * 2 * num_samples);
        uint8_t *data = (uint8_t*) allocator->malloc(packet_size);
        memset(data, 0, packet_size);
        RTP_Header *packet = (RTP_Header*) data;

        // initialise each RTP packet
        packet->set_version(2);
        packet->set_payload(get_payload_type());
        packet->set_timestamp(0);
        packet->set_ssrc(0);

        // save to the free list
        struct Block *block = new struct Block;
        block->next = 0;
        block->packet = packet;
        blocks.push(block, mutex);
    }
}

RTP_Engine::~RTP_Engine()
{
    while (true)
    {
        struct Block *block = blocks.pop(mutex);
        if (!block) break;
        allocator->free(block->packet);
        delete block;
    }
    delete mutex;
}

size_t RTP_Engine::rx_bytes()
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

#include "panglos/drivers/gpio.h"
#include "panglos/object.h"

int RTP_Engine::send(struct Block *block)
{
#if defined(ESP32)
    static panglos::GPIO *gpio = 0;
    static bool first = true;
    if (first && !gpio)
    {
        gpio = (panglos::GPIO*) panglos::Objects::objects->get("dbg");
        first = false;
    }
    if (gpio)
    {
        gpio->toggle();
    }
#endif

    if (block->samples > num_samples) return false;
    if (block->samples <= 0) return false;
    // increment the seq id
    block->packet->set_seq(packet_seq++);
    block->packet->set_timestamp(timestamp);
    timestamp += (int32_t) block->samples;
    // send the packet to all the clients in PLAY state
    size_t packet_size = sizeof(RTP_Header) + (sizeof(uint16_t) * 2 * block->samples);
    struct Buffer params = { .data = (uint8_t*) block->packet, .size = packet_size };
    playing.visit(send_packet, & params, mutex);

    // recyle the block
    put_free(block);
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
