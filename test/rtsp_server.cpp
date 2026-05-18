
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "rtsp_server.h"

TEST(RtspServer, Test)
{
    rtsp_server("127.0.0.1", "8554", 0);
}

//  FIN
