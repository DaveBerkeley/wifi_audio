
#include <gtest/gtest.h>

#include "audio_codec.h"

static struct OpusConfig config
{
    .bit_rate = 96000,
    .complexity = 8,
    .packet_rate = 20,
};

TEST(Opus, Test)
{
    AudioCodec *codec = AudioCodec::create(& config);

    int pt = codec->get_payload_type();
    EXPECT_EQ(97, pt);

    const char *name = codec->name();
    EXPECT_STREQ("Opus", name);

    delete codec;
}

//  FIN
