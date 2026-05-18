
#pragma once

#include "panglos/socket.h"
#include "panglos/io.h"

#include "rtsp.h"

class RTP_Engine;

void rtsp_server(const char *ip, const char *port, RTP_Engine *rtp);

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
    int last_error;
    int session_id;
    int session_version; // increment if values have changed ..
    const char *ip_addr;
    const char *port;
    SocketOut out;
    panglos::FmtOut fmt;

    int describe(const char *uri, RtspHeader *hdrs);
    int options(const char *uri, RtspHeader *hdrs);
    int send_error(RtspHeader *hdrs, int error_code);

public:
    RTSP_Handler(RTP_Engine *rtp, panglos::Socket *s, const char *ip, const char *port, int sid);

    virtual RtspCommand error(int code) override;
    virtual int get_last_error() override;
    virtual int command(RtspCommand cmd, const char *uri, RtspHeader *hdrs, int error_code) override;

    void set_socket(panglos::Socket *s);
};

//  FIN
