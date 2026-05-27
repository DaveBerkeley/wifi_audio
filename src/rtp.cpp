
#include <string.h>
#include <stdlib.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"
#include "panglos/verbose.h"
#include "panglos/time.h"

using namespace panglos;

#include "rtp.h"
#include "audio_codec.h"

static VERBOSE(rtp, "rtp", false);

    /*
     *
     */

RTP_Client::RTP_Client(panglos::Socket *s)
:   socket(s),
    num_packets(0),
    num_errors(0),
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

Allocator *Allocator::system()
{
    static SystemAllocator allocator;

    return & allocator;
}

    /*
     *
     */

RTP_Engine::RTP_Engine(AudioCodec *_codec, int rtp, int rtcp, int num_buffers, Allocator *a)
:   codec(_codec),
    rtp_port(rtp),
    rtcp_port(rtcp),
    allocator(a),
    playing(RTP_Client::get_next),
    mutex(0),
    packet_seq(1),
    timestamp(0),
    blocks(Block::get_next)
{
    ASSERT(codec);
    allocator = allocator ? allocator : Allocator::system();
    mutex = panglos::Mutex::create();

    // TODO : assumes zero CSRC and Extension blocks in header
    for (int idx = 0; idx < num_buffers; idx++)
    {
        size_t packet_size = sizeof(RTP_Header) + codec->max_payload_size();
        uint8_t *data = (uint8_t*) allocator->malloc(packet_size);
        memset(data, 0, packet_size);
        RTP_Header *packet = (RTP_Header*) data;

        // initialise each RTP packet
        packet->set_version(2);
        packet->set_payload(codec->get_payload_type());
        packet->set_timestamp(0);
        packet->set_ssrc(0);

        // save to the free list
        struct Block *block = new struct Block;
        block->next = 0;
        block->packet = packet;
        block->max_payload = codec->max_payload_size();
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
    ASSERT(codec);
    return codec->samples_per_packet() * codec->num_chans() * codec->data_size();
}

void RTP_Engine::get_server_ports(int *a, int *b)
{
    if (a) *a = rtp_port;
    if (b) *b = rtcp_port;
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

bool RTP_Engine::has_clients()
{
    return playing.head;
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
    const int sent = client->send(b->data, b->size);

    // log tx errors
    if (sent != (int) b->size)
    {
        client->error();
        static Time::tick_t last_error = 0;
        if (rtp.verbose)
        {
            if (Time::elapsed(last_error, 100))
            {
                last_error = Time::get();
                PO_ERROR("tx errors %d", client->get_num_errors());
            }
        }
    }
    return 0;
}

#if defined(ESP32)
#include "panglos/drivers/gpio.h"
#include "panglos/object.h"

static void TRACE()
{
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
}

#else

#define TRACE()

#endif

int RTP_Engine::send(struct Block *block, size_t bytes, size_t samples)
{
    TRACE();

    //PO_DEBUG("max_payload=%d bytes=%d", (int) block->max_payload, (int) bytes);
    ASSERT(block->max_payload >= bytes); // not enough space for payload
    // increment the seq id
    block->packet->set_seq(packet_seq++);
    block->packet->set_timestamp(timestamp);
    timestamp += (int32_t) samples;

    // send the packet to all the clients in PLAY state
    struct Buffer params = {
        .data = (uint8_t*) block->packet, 
        .size = sizeof(RTP_Header) + bytes,
    };
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
