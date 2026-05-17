

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

    virtual int describe(const char *uri, RtspHeaders *hdrs) override
    {
        PO_DEBUG("uri=%s hdr=%p", uri, hdrs);
        if (!hdrs->accept_sdp) return last_error = E_Not_Acceptable;

        return E_OK;
    }

    virtual int options(const char *uri, RtspHeaders *hdrs) override
    {
        PO_DEBUG("uri=%s hdr=%p", uri, hdrs);
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

    char buff[256];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_DESCRIBE);
    EXPECT_EQ(E_OK, handler.get_last_error());

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

    char buff[256];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_UNKNOWN);
    EXPECT_EQ(E_Not_Acceptable, handler.get_last_error());

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

    char buff[256];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_UNKNOWN);
    EXPECT_EQ(E_Version_Not_Supported, handler.get_last_error());

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

    char buff[256];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_UNKNOWN);
    EXPECT_EQ(E_Not_Implemented, handler.get_last_error());

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

    char buff[256];
    const size_t s = set_buff(buff, sizeof(buff), describe);

    int code = session->process(buff, s);
    EXPECT_EQ(code, C_DESCRIBE);
    EXPECT_EQ(E_OK, handler.get_last_error());

    delete session;
}

//  FIN
