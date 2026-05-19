
#pragma once

#include "panglos/socket.h"
#include "panglos/io.h"

#include "rtsp.h"
#include "rtp.h"

class RTP_Engine;

class SidGenerator 
{
public:
    virtual ~SidGenerator() { }
    virtual uint32_t generate() = 0;
};

void rtsp_server(const char *ip, const char *port, RTP_Engine *rtp, SidGenerator *gen=0);

    /*
     *
     */

class SocketOut : public panglos::Out
{
    panglos::Socket *socket;
    
public:
    SocketOut(panglos::Socket *s);

    virtual int tx(const char* data, int n) override;

    void set_socket(panglos::Socket *s);
};

    /*
     *
     */

class RTSP_Handler : public RTSP_Session::Handler
{
    RTP_Engine *rtp;
    int last_error;
    int session_id;
    int session_version; // increment if values have changed ..
    const char *ip_addr;
    const char *port;
    panglos::Socket *socket;
    char *buff;
    panglos::CharOut *out;
    panglos::FmtOut *fmt;
    RTP_Client *rtp_client;

    // command responses
    int describe(RtspHeader *hdrs);
    int common(RtspHeader *hdrs);
    int options(RtspHeader *hdrs);
    int setup(RtspHeader *hdrs);
    int play(RtspHeader *hdrs);
    int pause(RtspHeader *hdrs);
    int teardown(RtspHeader *hdrs);

    int send_error(RtspHeader *hdrs, int error_code);
    void flush();

    char *get_ip(panglos::Socket *);

public:
    RTSP_Handler(RTP_Engine *rtp, panglos::Socket *s, const char *ip, const char *port, int sid);
    ~RTSP_Handler();

    virtual RtspCommand error(int code) override;
    virtual int get_last_error() override;
    virtual int command(RtspCommand cmd, const char *uri, RtspHeader *hdrs, int error_code) override;
    virtual void terminate() override;

    void set_socket(panglos::Socket *s);
};

//  FIN
