
#pragma once

#include "panglos/list.h"
#include "panglos/socket.h"

    /*
     *
     */

static inline void set_u8(uint8_t *head, uint8_t data, uint8_t mask, uint8_t shift)
{
    uint8_t d = (*head) & uint8_t(~(mask << shift));
    d |= (data & mask) << shift;
    *head = d;
}

static inline void set_u16(uint8_t *head, uint16_t data)
{
    head[0] = uint8_t(data >> 8);
    head[1] = uint8_t(data);
}

static inline void set_u32(uint8_t *head, uint32_t data)
{
    head[0] = uint8_t(data >> 24);
    head[1] = uint8_t(data >> 16);
    head[2] = uint8_t(data >> 8);
    head[3] = uint8_t(data);
}

    /*
     *
     */

struct 
//__attribute__((packed)) 
RTP_Header
{
    uint8_t head[12];
    int16_t audio[0];

    void set_version(uint8_t v)
    {
        set_u8(& head[0], v, 0x03, 6);
    }
    void set_payload(uint8_t pt)
    {
        set_u8(& head[1], pt, 0x7f, 0);
    }
    void set_seq(uint16_t seq)
    {
        set_u16(& head[2], seq);
    }
    void set_timestamp(uint32_t ts)
    {
        set_u32(& head[4], ts);
    }
    void set_ssrc(uint32_t ssrc)
    {
        set_u32(& head[8], ssrc);
    }

    uint8_t *get_audio()
    {
        // TODO : check CSRC & Extension blocks
        return (uint8_t*) & audio[0];
    }
};

    /*
     *
     */

class RTP_Client
{
    panglos::Socket *socket;
    int num_packets;
    int num_errors;
    RTP_Client *next;

public:
    RTP_Client(panglos::Socket *s);
    ~RTP_Client();

    int send(const uint8_t *data, size_t len);
    
    panglos::Socket *get_socket() { return socket; }
    int get_num_packets() { return num_packets; }
    int get_num_errors() { return num_errors; }

    void error() { num_errors += 1; }

    static RTP_Client **get_next(RTP_Client *item) { return & item->next; }
};

    /*
     *
     */

class Allocator
{
public:
    virtual ~Allocator() { }

    virtual void* malloc(size_t bytes) = 0;
    virtual void free(void *) = 0;

    static Allocator* system();
};

    /*
     *
     */

class AudioCodec;

class RTP_Engine
{
    AudioCodec *codec;
    int rtp_port;
    int rtcp_port;
    Allocator *allocator;
    panglos::List<RTP_Client*> playing;
    panglos::Mutex *mutex;
    uint16_t packet_seq;
    uint32_t timestamp;

public:
    // 360 samples gives a 1496 byte packet, small enough to avoid fragmentation.
    const size_t num_samples = 360;

    struct Block
    {
        struct RTP_Header *packet;
        size_t samples;

        struct Block *next;
        static Block **get_next(struct Block *b) { return & b->next; }
    };

    typedef panglos::List<struct Block*> Blocks;

    Blocks blocks;

    RTP_Engine(AudioCodec *codec, int rtp_port, int rtcp_port, int num_buffers, Allocator *alloc=0);
    ~RTP_Engine();

    void get_server_ports(int *a, int *b);
    uint8_t get_payload_type();

    void play(RTP_Client *);
    void remove(RTP_Client *);

    int send(struct Block *block);
    struct Block *get_free() { return blocks.pop(mutex); }
    void put_free(struct Block *b) { blocks.push(b, mutex); }

    size_t rx_bytes();

    // Stats
    int get_num_clients();
    RTP_Client *get_client(int idx);
};

void make_1kHz(RTP_Engine *rtp, int gain);

//  FIN
