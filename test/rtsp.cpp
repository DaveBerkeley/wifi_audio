

#include <stddef.h>

#include <gtest/gtest.h>

#include "rtsp.h"

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

TEST(RTSP, Describe)
{
    RTSP_Session *session = RTSP_Session::create(0);

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

    session->process(buff, s);

    delete session;
}

//  FIN
