
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "panglos/debug.h"
#include "panglos/io.h"

#include "rtsp_server.h"

using namespace panglos;

    /*
     *
     */

class SocketOut : public panglos::Out
{
    panglos::Socket *socket;
public:
    SocketOut(panglos::Socket *s)
    :   socket(s)
    {
    }

    virtual int tx(const char* data, int n) override
    {
        ASSERT(socket);
        //printf("%.*s", n, data);
        //UNUSED(data);
        //return n;
        return socket->send((const uint8_t*) data, n);
    }

    void set_socket(Socket *s)
    {
        socket = s;
    }
};

    /*
     *
     */

class Handler : public RTSP_Session::Handler
{
    int last_error;
    int session_id;
    int session_version; // increment if values have changed ..
    const char *ip_addr;
    const char *port;
    SocketOut out;
    panglos::FmtOut fmt;

public:
    Handler(RTP_Engine *, panglos::Socket *s, const char *ip, const char *_port, int sid)
    :   last_error(E_OK),
        session_id(sid),
        session_version(1),
        ip_addr(ip),
        port(_port),
        out(s),
        fmt(& out)
    {
    }

    void set_socket(Socket *s)
    {
        out.set_socket(s);
        fmt.set(& out);
    }

    virtual RtspCommand error(int code) override
    {
        send_error(0, code);
        return C_UNKNOWN;
    }

    virtual int get_last_error() override
    {
        return last_error;
    }

    int describe(const char *uri, RtspHeader *hdrs)
    {
        UNUSED(uri);

        // SDP payload
        const char *sdp_fmt = 
        "v=0\r\n" // protocol version
        "o=- %d %d IN IP4 %s\r\n"
        "s=Audio Stream\r\n"
        "c=IN IP4 0.0.0.0\r\n" // connection data - fixed during SETUP phase
        "t=0 0\r\n" // timing 
        "m=audio 0 RTP/AVP 96\r\n" // media announcement : 96 = dynamic payload
        "a=rtpmap:96 L16/48000/2\r\n" // map the payload type
        ;

        char buff[1024];
        snprintf(buff, sizeof(buff), sdp_fmt, session_id, session_version, ip_addr);

        int code = E_OK;
        fmt.printf("RTSP/1.0 %d %s\r\n", code, lut(response_lut, code));
        fmt.printf("CSeq: %d\r\n", hdrs->cseq);
        fmt.printf("Content-Type: application/sdp\r\n");
        fmt.printf("Content-Length: %ld\r\n", strlen(buff));
        fmt.printf("\r\n");
        fmt.printf("%s", buff);
        return code;
    }

    int options(const char *uri, RtspHeader *hdrs)
    {
        UNUSED(uri);

        char buff[1024];

        char *s = buff;
        size_t sz = sizeof(buff);
        bool first = true;
        for (const LUT *_lut = cmd_lut; _lut->text; _lut++)
        {
            snprintf(s, sz, "%s%s", first ? "" : ", ", _lut->text);
            first = false;
            s += strlen(s);
            sz = sizeof(buff) - strlen(buff);
        }

        int code = E_OK;
        fmt.printf("RTSP/1.0 %d %s\r\n", code, lut(response_lut, code));
        fmt.printf("CSeq: %d\r\n", hdrs->cseq);
        fmt.printf("Public: %s\r\n", buff);
        fmt.printf("\r\n");
        return code;
    }

    int send_error(RtspHeader *hdrs, int error_code)
    {
        // Error Response
        fmt.printf("RTSP/1.0 %d %s\r\n", error_code, lut(response_lut, error_code));
        if (hdrs)
            fmt.printf("CSeq: %d\r\n", hdrs->cseq);
        fmt.printf("\r\n");
        return last_error = error_code;
    }

    virtual int command(RtspCommand cmd, const char *uri, RtspHeader *hdrs, int error_code) override
    {
        PO_DEBUG("cmd=%s uri=%s hdr=%p err=%d", lut(cmd_lut, cmd), uri, hdrs, error_code);

        if (error_code != E_OK)
        {
            // Error Response
            return send_error(hdrs, error_code);
        }

        switch (cmd)
        {
            case C_DESCRIBE : 
            {
                if (!hdrs->accept_sdp) return send_error(hdrs, E_Unsupported_Media_Type);
                return describe(uri, hdrs);
                break;
            }
            case C_OPTIONS :
            {
                return options(uri, hdrs);
                break;
            }
            case C_SETUP : 
                break;
            default : ASSERT(0);
        }

        return E_OK;
    }
};

    /*
     *
     */

class RtspClient : public Client
{
    RTP_Engine *rtp;
    Handler *handler;
    RTSP_Session *session;
    bool dead;

    virtual void run() override
    {
        PO_DEBUG("");
        ASSERT(sock);
        handler->set_socket(sock);

        // call in a loop : 
        // session->process(data, sz);
        while (!dead)
        {
            uint8_t buff[2048];

            int sz = sock->recv(buff, sizeof(buff));
            PO_DEBUG("sz=%d", sz);
            if (sz < 0) break;
            session->process((char*) buff, sz);
        }

        PO_DEBUG("DONE");
    }

public:
    RtspClient(SocketServer *ss, RTP_Engine *r, const char *ip, const char *port)
    :   Client(ss),
        rtp(r),
        handler(0),
        session(0),
        dead(false)
    {
        int session_id = 12345;
        handler = new Handler(rtp, sock, ip, port, session_id);
        session = RTSP_Session::create(handler);
    }

    ~RtspClient()
    {
        delete session;
        delete handler;
    }
};

    /*
     *
     */

class Factory : public Client::Factory
{
    RTP_Engine *rtp;
    const char *ip;
    const char *port;

    virtual Client *create_client(SocketServer *ss) override
    {
        return new RtspClient(ss, rtp, ip, port);
    }
public:
    Factory(RTP_Engine *r, const char *_ip, const char *_port)
    :   rtp(r),
        ip(strdup(_ip)),
        port(strdup(_port))
    {
    }

    ~Factory()
    {
        free((char*) ip);
        free((char*) port);
    }
};

    /*
     *
     */

void rtsp_server(const char *ip, const char *port, RTP_Engine *rtp)
{
    Socket *socket = Socket::open_tcpip(ip, port, Socket::SERVER);
    if (!socket)
    {
        PO_ERROR("unable to open socket(%s,%s)", ip, port);
        return;
    }
    Factory factory(rtp, ip, port);
    // blocking call to socket server
    run_socket_server(socket, & factory);
    delete socket;
}

//  FIN
