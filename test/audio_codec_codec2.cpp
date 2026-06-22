
#include "gtest/gtest.h"

#include "codec2/src/codec2.h"

#include "panglos/debug.h"

#include "audio_codec.h"

//using namespace panglos;

    /*
     *
     */

TEST(Codec2, Test)
{
    struct Codec2Config config =
    {
        .mode = CODEC2_MODE_3200,
        .fs = 8000,
    };
    AudioCodec *codec = AudioCodec::create(& config);

    size_t s = codec->samples_per_packet();
    PO_DEBUG("%d", (int) s); 

    delete codec;
}

//  FIN
