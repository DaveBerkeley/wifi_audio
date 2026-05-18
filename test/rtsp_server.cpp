
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "rtsp_server.h"
#include "rtp.h"

TEST(RtspServer, Test)
{
    RTP_Engine rtp(6000, 6001);
    rtsp_server("127.0.0.1", "8554", & rtp);
}

//  FIN
