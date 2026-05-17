

#include <stddef.h>

#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "rtsp.h"

static size_t set_buff(char *buff, size_t s, const char **lines)
{
    size_t total = 0;
    for (; *lines; lines++)
    {
        const char *line = *lines;
        snprintf(buff, s, "%s\r\n", line); // RTSP/2.0 all lines end with "\r\n"
        const size_t size = strlen(buff);
        buff += size;
        s -= size;
        total += size;
    }
    return total;
}

    /*
     *
     */

class Handler : public RTSP_Session::Handler
{
    int last_error;
public:
    Handler() : last_error(E_OK) { }

    virtual RtspCommand error(int code) override
    {
        last_error = code;
        PO_DEBUG("code=%d", code);
        return C_UNKNOWN;
    }

    virtual int get_last_error() override
    {
        return last_error;
    }

    virtual int command(RtspCommand cmd, const char *uri, RtspHeader *hdrs) override
    {
        PO_DEBUG("cmd=%s uri=%s hdr=%p", lut(cmd_lut, cmd), uri, hdrs);
        switch (cmd)
        {
            case C_DESCRIBE : 
            {
                if (!hdrs->accept_sdp) return last_error = E_Unsupported_Media_Type;
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

TEST(RTSP, Describe)
{
    Handler handler;
    RTSP_Session *session = RTSP_Session::create(& handler);

    const char* describe[] = {
        "DESCRIBE rtsp://server.example.com/fizzle/foo RTSP/2.0",
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

    delete session;
}

TEST(RTSP, NoSdp)
{
    Handler handler;
    RTSP_Session *session = RTSP_Session::create(& handler);

    const char* describe[] = {
        "DESCRIBE rtsp://server.example.com/fizzle/foo RTSP/2.0",
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

    delete session;
}

TEST(RTSP, WrongVersion)
{
    Handler handler;
    RTSP_Session *session = RTSP_Session::create(& handler);

    const char* describe[] = {
        "DESCRIBE rtsp://server.example.com/fizzle/foo RTSP/1.0",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_UNKNOWN);
    EXPECT_EQ(E_Version_Not_Supported, handler.get_last_error());
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    delete session;
}

TEST(RTSP, BadCommand)
{
    Handler handler;
    RTSP_Session *session = RTSP_Session::create(& handler);

    const char* describe[] = {
        "DESCRIB rtsp://server.example.com/fizzle/foo RTSP/2.0",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_UNKNOWN);
    EXPECT_EQ(E_Bad_Request, handler.get_last_error());
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    delete session;
}

TEST(RTSP, IgnoreLeading)
{
    Handler handler;
    RTSP_Session *session = RTSP_Session::create(& handler);

    const char* describe[] = {
        "",
        "",
        "",
        "DESCRIBE rtsp://server.example.com/fizzle/foo RTSP/2.0",
        "",
        0,
    };

    char buff[1024];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_DESCRIBE);
    EXPECT_EQ(E_OK, handler.get_last_error());
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    delete session;
}

TEST(RTSP, Setup)
{
    Handler handler;
    RTSP_Session *session = RTSP_Session::create(& handler);
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    const char* describe[] = {
        "SETUP rtsp://server.example.com/fizzle/foo RTSP/2.0",
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

    delete session;
}

TEST(RTSP, SetupComplex)
{
    Handler handler;
    RTSP_Session *session = RTSP_Session::create(& handler);
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    const char* describe[] = {
        "SETUP rtsp://server.example.com/fizzle/foo RTSP/2.0",
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

    delete session;
}

TEST(RTSP, SetupComma)
{
    Handler handler;
    RTSP_Session *session = RTSP_Session::create(& handler);
    EXPECT_EQ(RTSP_Session::INIT, session->get_state());

    const char* describe[] = {
        "SETUP rtsp://server.example.com/fizzle/foo RTSP/2.0",
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

    delete session;
}

//  FIN
