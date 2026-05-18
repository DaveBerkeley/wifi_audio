
#pragma once

#include <arpa/inet.h>

#include "panglos/list.h"
#include "panglos/socket.h"

    /*
     *
     */

inline void set_u32(uint32_t *data, uint32_t mask, uint32_t shift, uint32_t value)
{
    value &= mask;
    mask <<= shift;
    uint32_t d = ntohl(*data);
    d &= mask;
    d |= value << shift;
    *data = htonl(d);
}

    /*
     *
     */

struct RTP_Header
{
    uint32_t head;
    uint32_t timestamp;
    uint32_t ssrc;
    uint16_t audio[0];

    void set_version(uint8_t v)
    {
        set_u32(& head, 0x3, 30, v);
    }
    void set_payload(uint8_t pt)
    {
        set_u32(& head, 0x7f, 16, pt);
    }
    void set_seq(uint16_t seq)
    {
        set_u32(& head, 0xffff, 0, seq);
    }
    void set_timestamp(uint32_t ts)
    {
        timestamp = htonl(ts);
    }
    void set_ssrc(uint32_t ssrc)
    {
        ssrc = htonl(ssrc);
    }

    void set_pcm(int idx, int lr, uint16_t data)
    {
        int s = (idx * 2) + lr;
        audio[s] = htons(data);
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

public:
    RTP_Engine(int rtp_port, int rtcp_port);
    ~RTP_Engine();

    void get_server_ports(int *a, int *b);
    int get_payload_type();

    void play(RTP_Client *);
    void pause(RTP_Client *);
    void remove(RTP_Client *);
};

//  FIN
