
#include <gtest/gtest.h>

#include "audio_codec.h"

static struct OpusConfig config
{
    .bits = 16,
    .chans = 2,
    .freq = 48000,
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
