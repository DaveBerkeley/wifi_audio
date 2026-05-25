
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "panglos/debug.h"

#include "sockets.h"

#include "rtsp_server.h"
#include "rtp.h"
#include "audio_codec.h"

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

RTSP_Handler::RTSP_Handler(RTP_Engine *_rtp, AudioCodec *_codec,
        panglos::Socket *s, const char *ip, const char *_port, int sid)
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
    rtp_client(0),
    codec(_codec)
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
    delete rtp_client;
}

void RTSP_Handler::terminate()
{
    rtp->remove(rtp_client);
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
    ASSERT(codec);
    const char *sdp_fmt = codec->get_sdp_fmt();

    ASSERT(rtp);
    size_t sz = 1024;
    char *buff = (char*) malloc(sz);
    snprintf(buff, sz, sdp_fmt, session_id, session_version, ip_addr);

    int code = E_OK;
    ASSERT(fmt);
    PO_DEBUG("%d %s", code, lut(response_lut, code));
    fmt->printf("RTSP/1.0 %d %s\r\n", code, lut(response_lut, code));
    fmt->printf("CSeq: %d\r\n", hdrs->cseq);
    fmt->printf("Content-Type: application/sdp\r\n");
    fmt->printf("Content-Length: %ld\r\n", strlen(buff));
    fmt->printf("\r\n");
    for (char *s = buff; *s; s++)
        fmt->tx(*s);
    flush();
    free(buff);
    return code;
}

int RTSP_Handler::options(RtspHeader *hdrs)
{
    int code = E_OK;
    ASSERT(fmt);
    PO_DEBUG("%d %s", code, lut(response_lut, code));
    fmt->printf("RTSP/1.0 %d %s\r\n", code, lut(response_lut, code));
    fmt->printf("CSeq: %d\r\n", hdrs->cseq);
    // Print all the supported commands, with ',' separators
    fmt->printf("Public: ");
    const char *sep = "";
    for (const LUT *_lut = cmd_lut; _lut->text; _lut++)
    {
        fmt->printf("%s%s", sep, _lut->text);
        sep = ", ";
    }
    fmt->printf("\r\n\r\n");
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

    /*
     *
     */

int RTSP_Handler::common(RtspHeader *hdrs)
{
    int code = E_OK;
    ASSERT(fmt);
    PO_DEBUG("%d %s", code, lut(response_lut, code));
    fmt->printf("RTSP/1.0 %d %s\r\n", code, lut(response_lut, code));
    fmt->printf("CSeq: %d\r\n", hdrs->cseq);
    fmt->printf("Session: %d\r\n", session_id);
    return code;
}

    /*
     *
     */

int RTSP_Handler::setup(RtspHeader *hdrs)
{
    // search for a usable transport
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

    int code = common(hdrs);
    int rtp_port = 6000, rtcp_port= 6001;
    ASSERT(rtp);
    rtp->get_server_ports(& rtp_port, & rtcp_port);
    fmt->printf("Transport: RTP/AVP;unicast;server_port=%d-%d;session=%d\r\n", 
        rtp_port, rtcp_port, session_id);
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

    int code = common(hdrs);
    fmt->printf("RTP-Info: url=rtsp://%s/media/trackID=0;seq=1;rtptime=0\r\n", ip_addr);
    fmt->printf("\r\n");
    flush();

    ASSERT(rtp);
    rtp->play(rtp_client);

    return code;
}

int RTSP_Handler::teardown(RtspHeader *hdrs)
{
    int code = common(hdrs);
    fmt->printf("\r\n");
    flush();

    ASSERT(rtp);
    rtp->remove(rtp_client);

    return code;
}

int RTSP_Handler::pause(RtspHeader *hdrs)
{
    int code = common(hdrs);
    fmt->printf("\r\n");
    flush();

    ASSERT(rtp);
    rtp->remove(rtp_client);

    return code;
}

    /*
     *
     */

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
    PO_DEBUG("%s %s %d", lut(cmd_lut, cmd), uri, error_code);

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
        case C_PAUSE : 
        {
            return pause(hdrs);
        }
        case C_TEARDOWN : 
        {
            return teardown(hdrs);
        }
        default : ASSERT(0);
    }

    return E_OK;
}

    /*
     *
     */

#if 0
static void dump(uint8_t *buff, int sz)
{
    int addr = 0;
    while (sz)
    {
        printf("%04x  ", addr);
        int block = (sz > 16) ? 16 : sz;
        for (int i = 0; i < block; i++)
        {
            printf("%s %02x", (i == 8) ? " " : "", *buff++);
        }
        printf("\n");
        sz -= block;
        addr += sz;
    }
    fflush(stdout);
}
#else
#define dump(a, b)
#endif

class RtspClient : public Client
{
    RTP_Engine *rtp;
    RTSP_Handler *handler;
    RTSP_Session *session;
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
        while (session->get_state() != RTSP_Session::DEAD)
        {
            int size = sock->recv((uint8_t*) & buff[idx], sz - idx);
            PO_DEBUG("size=%d idx=%d sum=%d", size, idx, size + idx);
            if (size <= 0) break;

            // check for complete message
            char *block = search("\r\n\r\n");
            if (!block)
            {
                idx += size;
                dump((uint8_t*) buff, idx);
                continue;
            }

            char *content = search("Content-Length: ");
            PO_DEBUG("block=%p content=%p", block, content);

            size_t end = 0;
            if (content)
            {
                // TODO: need to parse the Content-Length and wait for the whole message
                ASSERT(0);
            }
            else
            {
                // skip the trailing "\r\n\r\n"
                end = (block + 4) - buff;
            }

            ASSERT(end);
            dump((uint8_t*) buff, idx);
            PO_DEBUG("process size=%ld", end);
            session->process(buff, end);

            // Need to copy any trailing buffer data to the start of the buffer
            idx = 0;
        }

        handler->terminate();
        PO_DEBUG("DONE");
    }

public:
    RtspClient(SocketServer *ss, RTP_Engine *r, AudioCodec *codec, const char *ip, const char *port, uint32_t sid)
    :   Client(ss),
        rtp(r),
        handler(0),
        session(0),
        buff(0),
        sz(2048),
        idx(0)
    {
        buff = new char[sz];
        memset(buff, 0, sz);

        ASSERT(codec);
        PO_DEBUG("codec=%s", codec->name());
        handler = new RTSP_Handler(rtp, codec, sock, ip, port, sid);
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
    SidGenerator *sid_gen;
    AudioCodec *codec;

    virtual Client *create_client(SocketServer *ss) override
    {
        uint32_t sid = sid_gen ? sid_gen->generate() : 12345;
        return new RtspClient(ss, rtp, codec, ip, port, sid);
    }
public:
    Factory(RTP_Engine *r, AudioCodec *_codec, const char *_ip, const char *_port, SidGenerator *gen=0)
    :   rtp(r),
        ip(strdup(_ip)),
        port(strdup(_port)),
        sid_gen(gen),
        codec(_codec)
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

void rtsp_server(const char *ip, const char *port, RTP_Engine *rtp, AudioCodec *codec, SidGenerator *gen)
{
    Socket *socket = Socket::open_tcpip(ip, port, Socket::SERVER);
    if (!socket)
    {
        PO_ERROR("unable to open socket(%s,%s)", ip, port);
        return;
    }
    Factory factory(rtp, codec, ip, port, gen);
    // blocking call to socket server
    run_socket_server(socket, & factory);
    delete socket;
}

//  FIN
