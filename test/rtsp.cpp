

#include <stddef.h>

#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/socket.h"
#include "panglos/io.h"

#include "rtsp.h"
#include "rtsp_server.h"

static size_t set_buff(char *buff, size_t s, const char **lines)
{
    size_t total = 0;
    for (; *lines; lines++)
    {
        const char *line = *lines;
        snprintf(buff, s, "%s\r\n", line);
        const size_t size = strlen(buff);
        buff += size;
        s -= size;
        total += size;
    }
    return total;
}

class TestSocketOut : public panglos::Out
{
    panglos::Socket *socket;
public:
    TestSocketOut(panglos::Socket *s)
    :   socket(s)
    {
    }

    virtual int tx(const char* data, int n) override
    {
        return socket->send((const uint8_t*) data, n);
    }
};

    /*
     *
     */

class TestSocket : public panglos::Socket
{
    char *buff;
    size_t sz;
    size_t idx;
public:
    virtual int send(const uint8_t *data, size_t len) override
    {
        snprintf(& buff[idx], sz - idx, "%.*s", (int) len, data);
        idx += len;
        ASSERT(idx < sz);
        return (int) len;
    }

    virtual int recv(uint8_t *data, size_t len) override
    {
        UNUSED(data);
        ASSERT(0);
        return (int) len;
    }

    TestSocket(size_t _sz=1024)
    :   buff(0),
        sz(_sz),
        idx(0)
    {
        buff = (char*) malloc(sz);
        memset(buff, 0, sz);
    }

    ~TestSocket()
    {
        free(buff);
    }

    char *get_buff() { return buff; }
};

    /*
     *
     */

const char *ip_addr = "127.0.0.1";
const char *port = "1234";
const int sid = 12345;

TEST(RTSP, Describe)
{
    TestSocket socket;
    RTSP_Handler handler(0, & socket, ip_addr, port, sid);
    RTSP_Session *session = RTSP_Session::create(& handler);

    const char* describe[] = {
        "DESCRIBE rtsp://1.2.3.4:1234/stream RTSP/1.0",
        "CSeq: 312",
        "User-Agent: PhonyClient/1.2",
        "Accept: application/example ;q=0.7, application/sdp",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_DESCRIBE);
    EXPECT_EQ(E_OK, handler.get_last_error());
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    EXPECT_STREQ(
        "RTSP/1.0 200 OK\r\n"
        "CSeq: 312\r\n"
        "Content-Type: application/sdp\r\n"
        "Content-Length: 123\r\n"
        "\r\n"
        "v=0\r\n"
        "o=- 12345 1 IN IP4 127.0.0.1\r\n"
        "s=Audio Stream\r\n"
        "c=IN IP4 0.0.0.0\r\n"
        "t=0 0\r\n"
        "m=audio 0 RTP/AVP 96\r\n"
        "a=rtpmap:96 L16/48000/2\r\n"
        "",
        socket.get_buff());

    delete session;
}

TEST(RTSP, NoSdp)
{
    TestSocket socket;
    RTSP_Handler handler(0, & socket, ip_addr, port, sid);
    RTSP_Session *session = RTSP_Session::create(& handler);

    const char* describe[] = {
        "DESCRIBE rtsp://1.2.3.4:1234/stream RTSP/1.0",
        "Accept: application/example",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_UNKNOWN);
    EXPECT_EQ(E_Unsupported_Media_Type, handler.get_last_error());
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    printf("%s", socket.get_buff());
    delete session;
}

TEST(RTSP, WrongVersion)
{
    TestSocket socket;
    RTSP_Handler handler(0, & socket, ip_addr, port, sid);
    RTSP_Session *session = RTSP_Session::create(& handler);

    const char* describe[] = {
        "DESCRIBE rtsp://1.2.3.4:1234/stream RTSP/2.0",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_UNKNOWN);
    EXPECT_EQ(E_Version_Not_Supported, handler.get_last_error());
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    printf("%s", socket.get_buff());
    delete session;
}

TEST(RTSP, BadCommand)
{
    TestSocket socket;
    RTSP_Handler handler(0, & socket, ip_addr, port, sid);
    RTSP_Session *session = RTSP_Session::create(& handler);

    const char* describe[] = {
        "DESCRIB rtsp://1.2.3.4:1234/stream RTSP/1.0",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_UNKNOWN);
    EXPECT_EQ(E_Bad_Request, handler.get_last_error());
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    printf("%s", socket.get_buff());
    delete session;
}

TEST(RTSP, IgnoreLeading)
{
    TestSocket socket;
    RTSP_Handler handler(0, & socket, ip_addr, port, sid);
    RTSP_Session *session = RTSP_Session::create(& handler);

    const char* describe[] = {
        "",
        "",
        "",
        "DESCRIBE rtsp://1.2.3.4:1234/stream RTSP/1.0",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_DESCRIBE);
    EXPECT_EQ(E_OK, handler.get_last_error());
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    printf("%s", socket.get_buff());
    delete session;
}

TEST(RTSP, Setup)
{
    TestSocket socket;
    RTSP_Handler handler(0, & socket, ip_addr, port, sid);
    RTSP_Session *session = RTSP_Session::create(& handler);
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    const char* describe[] = {
        "SETUP rtsp://1.2.3.4:1234/stream RTSP/1.0",
        "Transport: RTP/AVP;unicast;client_port=5000-5001",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_SETUP);
    EXPECT_EQ(E_OK, handler.get_last_error());
    // check the state transition
    EXPECT_EQ(RTSP_Session::READY, session->get_state());

    printf("%s", socket.get_buff());
    delete session;
}

TEST(RTSP, SetupComplex)
{
    TestSocket socket;
    RTSP_Handler handler(0, & socket, ip_addr, port, sid);
    RTSP_Session *session = RTSP_Session::create(& handler);
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    const char* describe[] = {
        "SETUP rtsp://1.2.3.4:1234/stream RTSP/1.0",
        "Transport: RTP/AVP/TCP;unicast;interleaved=0-1;mode=PLAY;rtcp-mux, RTP/AVP/UDP;unicast;client_port=4588-4589;ssrc=6095d7d7;mode=PLAY, RTP/AVP;multicast;destination=225.219.201.15;port=7000-7001;ttl=16",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_SETUP);
    EXPECT_EQ(E_OK, handler.get_last_error());
    // check the state transition
    EXPECT_EQ(RTSP_Session::READY, session->get_state());

    printf("%s", socket.get_buff());
    delete session;
}

TEST(RTSP, SetupComma)
{
    TestSocket socket;
    RTSP_Handler handler(0, & socket, ip_addr, port, sid);
    RTSP_Session *session = RTSP_Session::create(& handler);
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    const char* describe[] = {
        "SETUP rtsp://1.2.3.4:1234/stream RTSP/1.0",
        "Transport: RTP/AVP;unicast;src_addr=192.168.1.1,224.0.0.1;dest_addr=224.0.0.1,192.168.1.2",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_SETUP);
    EXPECT_EQ(E_OK, handler.get_last_error());
    // check the state transition
    EXPECT_EQ(RTSP_Session::READY, session->get_state());

    printf("%s", socket.get_buff());
    delete session;
}

TEST(RTSP, Options)
{
    TestSocket socket;
    RTSP_Handler handler(0, & socket, ip_addr, port, sid);
    RTSP_Session *session = RTSP_Session::create(& handler);
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    const char* describe[] = {
        "OPTIONS rtsp://1.2.3.4:1234/stream RTSP/1.0",
        "CSeq: 1234",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_OPTIONS);
    EXPECT_EQ(E_OK, handler.get_last_error());
    // check the state transition
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    EXPECT_STREQ(
        "RTSP/1.0 200 OK\r\n"
        "CSeq: 1234\r\n"
        "Public: OPTIONS, DESCRIBE, SETUP, PAUSE, PLAY, TEARDOWN\r\n"
        "\r\n",
        socket.get_buff());

    delete session;
}

//  FIN
