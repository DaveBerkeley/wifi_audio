
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "panglos/debug.h"

#include "rtsp_server.h"
#include "rtp.h"

using namespace panglos;

    /*
     *
     */

SocketOut::SocketOut(panglos::Socket *s)
:   socket(s)
{
}

int SocketOut::tx(const char* data, int n)
{
    ASSERT(socket);
    return socket->send((const uint8_t*) data, n);
}

void SocketOut::set_socket(Socket *s)
{
    socket = s;
}

    /*
     *
     */

RTSP_Handler::RTSP_Handler(RTP_Engine *_rtp, panglos::Socket *s, const char *ip, const char *_port, int sid)
:   rtp(_rtp),
    last_error(E_OK),
    session_id(sid),
    session_version(1),
    ip_addr(ip),
    port(_port),
    socket(s),
    buff(0),
    out(0),
    fmt(0),
    rtp_client(0)
{
    const int sz = 1024;
    buff = new char[sz];
    out = new CharOut(buff, sz);
    fmt = new FmtOut(out);
}

RTSP_Handler::~RTSP_Handler()
{
    delete fmt;
    delete out;
    delete[] buff;
    rtp->remove(rtp_client);
    delete rtp_client;
}

void RTSP_Handler::set_socket(Socket *s)
{
    socket = s;
}

RtspCommand RTSP_Handler::error(int code)
{
    send_error(0, code);
    return C_UNKNOWN;
}

int RTSP_Handler::get_last_error()
{
    return last_error;
}

void RTSP_Handler::flush()
{
    PO_DEBUG("sz=%d", out->get_idx());
    socket->send((uint8_t*) buff, out->get_idx());
    out->reset();
}

    /*
     *  Command handlers : send response
     */

int RTSP_Handler::describe(RtspHeader *hdrs)
{
    // SDP payload
    const char *sdp_fmt = 
        "v=0\r\n" // protocol version
        "o=- %d %d IN IP4 %s\r\n"
        "s=Audio Stream\r\n"
        "c=IN IP4 0.0.0.0\r\n" // connection data - fixed during SETUP phase
        "t=0 0\r\n" // timing 
        "m=audio 0 RTP/AVP %d\r\n" // media announcement : 96 = dynamic payload
        "a=rtpmap:%d L16/48000/2\r\n" // map the payload type
        ;

    ASSERT(rtp);
    char buff[1024];
    int pt = rtp->get_payload_type();
    snprintf(buff, sizeof(buff), sdp_fmt, session_id, session_version, ip_addr, pt, pt);

    int code = E_OK;
    ASSERT(fmt);
    PO_DEBUG("%d %s", code, lut(response_lut, code));
    fmt->printf("RTSP/1.0 %d %s\r\n", code, lut(response_lut, code));
    fmt->printf("CSeq: %d\r\n", hdrs->cseq);
    fmt->printf("Content-Type: application/sdp\r\n");
    fmt->printf("Content-Length: %ld\r\n", strlen(buff));
    fmt->printf("\r\n");
    fmt->printf("%s", buff);
    flush();
    return code;
}

int RTSP_Handler::options(RtspHeader *hdrs)
{
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
    ASSERT(fmt);
    PO_DEBUG("%d %s", code, lut(response_lut, code));
    fmt->printf("RTSP/1.0 %d %s\r\n", code, lut(response_lut, code));
    fmt->printf("CSeq: %d\r\n", hdrs->cseq);
    fmt->printf("Public: %s\r\n", buff);
    fmt->printf("\r\n");
    flush();
    return code;
}

char *RTSP_Handler::get_ip(panglos::Socket *)
{
    int fd = socket->get_fd();

    if (fd <= 0)
    {
        const char *ip = "1.2.3.4";
        PO_DEBUG("simulated ip=%s", ip);
        return strdup(ip);
    }

    // We have a real socket : not a simulation
    struct sockaddr_in addr;
    memset(& addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    int err = getpeername(fd, (struct sockaddr*) & addr, & len);
    if (err < 0)
    {
        PO_ERROR("getpeername() err=%d '%s'", errno, strerror(errno));
        return 0;
    }

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, & addr.sin_addr, ip, sizeof(ip));
    PO_DEBUG("ip=%s", ip);
    return strdup(ip);
}

int RTSP_Handler::setup(RtspHeader *hdrs)
{
    // Check for a usable transport
    RtspHeader::Transport *transport = 0;
    for (int i = 0; i < RtspHeader::MAX_TRANSPORTS; i++)
    {
        RtspHeader::Transport *t = & hdrs->transport[i];
        if (!t->unicast)                    continue;
        if (!t->rtp)                        continue;
        if (strcmp("PLAY", t->mode))        continue;
        if (strcmp("UDP", t->transport))    continue;
        if (t->client_port[0] == 0)         continue;
        if (t->client_port[1] == 0)         continue;
        //PO_DEBUG("found valid transport");
        transport = t;
        break;
    }

    if (!transport)
    {
        PO_ERROR("no supported transport");
        return send_error(hdrs, E_Unsupported_Transport);
    }

    int code = E_OK;
    ASSERT(fmt);
    PO_DEBUG("%d %s", code, lut(response_lut, code));
    fmt->printf("RTSP/1.0 %d %s\r\n", code, lut(response_lut, code));
    fmt->printf("CSeq: %d\r\n", hdrs->cseq);
    int rtp_port = 6000, rtcp_port= 6001;
    ASSERT(rtp);
    rtp->get_server_ports(& rtp_port, & rtcp_port);
    fmt->printf("Transport: RTP/AVP;unicast;server_port=%d-%d;session=%d\r\n", 
        rtp_port, rtcp_port, session_id);
    fmt->printf("Session: %d\r\n", session_id);
    fmt->printf("\r\n");
    flush();

    // create the client
    char port[16];
    snprintf(port, sizeof(port), "%d", rtp_port);
    PO_DEBUG("ip=%s:%s", ip_addr, port);
    // server socket will bind() to the local ip,port
    panglos::Socket *client_socket = panglos::Socket::open_udp(ip_addr, port, panglos::Socket::SERVER);

    snprintf(port, sizeof(port), "%d", transport->client_port[0]);
    char *ip = get_ip(socket);
    PO_DEBUG("ip=%s:%s", ip, port);
    client_socket->connect(ip, port);
    free(ip);

    if (rtp_client)
    {
        PO_ERROR("existing rtp client! %p", rtp_client);
        rtp->remove(rtp_client);
    }
    rtp_client = new RTP_Client(client_socket);

    return code;
}

int RTSP_Handler::play(RtspHeader *hdrs)
{
    if (!rtp_client)
    {
        // ERROR STATE
        ASSERT(0);
    }

    int code = E_OK;
    ASSERT(fmt);
    PO_DEBUG("%d %s", code, lut(response_lut, code));
    fmt->printf("RTSP/1.0 %d %s\r\n", code, lut(response_lut, code));
    fmt->printf("CSeq: %d\r\n", hdrs->cseq);
    fmt->printf("Session: %d\r\n", hdrs->session_id);
    fmt->printf("RTP-Info: url=rtsp://%s/media.mp4/trackID=0;seq=1;rtptime=0\r\n",
        ip_addr);
    fmt->printf("\r\n");
    flush();

    ASSERT(rtp);
    rtp->play(rtp_client);

    return code;
}

int RTSP_Handler::send_error(RtspHeader *hdrs, int error_code)
{
    // Error Response
    ASSERT(fmt);
    PO_DEBUG("%d %s", error_code, lut(response_lut, error_code));
    fmt->printf("RTSP/1.0 %d %s\r\n", error_code, lut(response_lut, error_code));
    if (hdrs)
        fmt->printf("CSeq: %d\r\n", hdrs->cseq);
    fmt->printf("\r\n");
    flush();
    return last_error = error_code;
}

int RTSP_Handler::command(RtspCommand cmd, const char *uri, RtspHeader *hdrs, int error_code)
{
    PO_DEBUG("cmd=%s uri=%s err=%d", lut(cmd_lut, cmd), uri, error_code);

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
            return describe(hdrs);
        }
        case C_OPTIONS :
        {
            return options(hdrs);
        }
        case C_SETUP : 
        {
            return setup(hdrs);
        }
        case C_PLAY : 
        {
            return play(hdrs);
        }
        default : ASSERT(0);
    }

    return E_OK;
}

    /*
     *
     */

class RtspClient : public Client
{
    RTP_Engine *rtp;
    RTSP_Handler *handler;
    RTSP_Session *session;
    bool dead;
    char *buff;
    int sz;
    int idx;

    char *search(const char *match)
    {
        return strstr(buff, match);
    }

    virtual void run() override
    {
        PO_DEBUG("");
        ASSERT(sock);
        handler->set_socket(sock);

        // call in a loop : 
        // session->process(data, sz);
        while (!dead)
        {
            int size = sock->recv((uint8_t*) & buff[idx], sz - idx);
            PO_DEBUG("size=%d idx=%d", size, idx);
            if (size <= 0) break;

            // check for complete message
            char *block = search("\r\n\r\n");
            if (!block)
            {
                idx += size;
                continue;
            }

            char *content = search("Content-Length: ");
            PO_DEBUG("block=%p content=%p", block, content);

            size_t end = 0;
            if (content)
            {
                // TODO:
                ASSERT(0);
            }
            else
            {
                end = (block + 4) - buff;
            }

            ASSERT(end);
            session->process(buff, end);
        }

        PO_DEBUG("DONE");
    }

public:
    RtspClient(SocketServer *ss, RTP_Engine *r, const char *ip, const char *port)
    :   Client(ss),
        rtp(r),
        handler(0),
        session(0),
        dead(false),
        buff(0),
        sz(2048),
        idx(0)
    {
        buff = new char[sz];
        memset(buff, 0, sz);

        int session_id = 12345;
        handler = new RTSP_Handler(rtp, sock, ip, port, session_id);
        session = RTSP_Session::create(handler);
    }

    ~RtspClient()
    {
        delete session;
        delete handler;
        delete[] buff;
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
