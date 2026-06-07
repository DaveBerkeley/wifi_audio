
#include <gtest/gtest.h>

#include "audio_codec.h"
#include "wav_file.h"
#include "i2s.h"
#include "utils.h"

#include "rtp.h"

TEST(RTP, HeaderFields)
{
    struct RTP_Header header = { { 0 } };

    header.set_version(0);
    EXPECT_EQ(header.head[0], 0x00);
    header.set_version(1);
    EXPECT_EQ(header.head[0], 0x40);
    header.set_version(2);
    EXPECT_EQ(header.head[0], 0x80);
    header.set_version(3);
    EXPECT_EQ(header.head[0], 0xc0);
    header.set_version(4);
    EXPECT_EQ(header.head[0], 0x00);

    header.set_payload(100);
    EXPECT_EQ(header.head[1], 100);
    header.set_payload(129);
    EXPECT_EQ(header.head[1], 1);

    header.set_seq(0x1234);
    EXPECT_EQ(header.head[2], 0x12);
    EXPECT_EQ(header.head[3], 0x34);

    header.set_timestamp(0x12345678);
    EXPECT_EQ(header.head[4], 0x12);
    EXPECT_EQ(header.head[5], 0x34);
    EXPECT_EQ(header.head[6], 0x56);
    EXPECT_EQ(header.head[7], 0x78);

    header.set_ssrc(0x01234567);
    EXPECT_EQ(header.head[8],  0x01);
    EXPECT_EQ(header.head[9],  0x23);
    EXPECT_EQ(header.head[10], 0x45);
    EXPECT_EQ(header.head[11], 0x67);
}

    /*
     *
     */

static void on_done(void *arg)
{
    EXPECT_TRUE(arg);
    bool *dead = (bool*) arg;
    *dead = true;
}

TEST(RTP, AudioCopy)
{
    OpusConfig config = {
        .bit_rate = 96000,
        .complexity = 8,
        .packet_rate = 60,
        .encode = true,
        .decode = false,
    };
    AudioCodec *codec = AudioCodec::create(& config);
    RTP_Engine *rtp = new RTP_Engine(codec, 6001, 6002, 8, 0);

    WavSource wav;
    wav.open("sine.wav");
    bool dead = false;
    wav.set_on_done(on_done, & dead);

    audio_copy(& wav, rtp, & dead);

    delete rtp;
    delete codec;
}

//  FIN
