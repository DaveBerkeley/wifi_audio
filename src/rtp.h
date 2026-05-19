
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
    uint16_t audio[0];

    void set_version(uint8_t v)
    {
        set_u8(& head[0], v, 0x03, 6);
    }
    void set_payload(uint8_t pt)
    {
        set_u8(& head[1], 0x7f, 0, pt);
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
};

    /*
     *
     */

class RTP_Client
{
    int rtp_port;
    RTP_Client *next;

    panglos::Socket *socket;
public:
    RTP_Client(panglos::Socket *s);
    ~RTP_Client();

    panglos::Socket *get_socket() { return socket; }

    static RTP_Client **get_next(RTP_Client *item) { return & item->next; }
};

    /*
     *
     */

class RTP_Engine
{
    int rtp_port;
    int rtcp_port;
    struct RTP_Header *packet;
    panglos::List<RTP_Client*> playing;
    panglos::Mutex *mutex;
    uint16_t packet_seq;
    uint32_t timestamp;

public:
    const int num_samples = 192;

    RTP_Engine(int rtp_port, int rtcp_port);
    ~RTP_Engine();

    void get_server_ports(int *a, int *b);
    uint8_t get_payload_type();

    void play(RTP_Client *);
    void pause(RTP_Client *);
    void remove(RTP_Client *);

    int send(int samples);

    uint16_t *rx_buff();
    size_t rx_size();
};

//  FIN
