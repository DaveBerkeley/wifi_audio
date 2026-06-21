
#include "gtest/gtest.h"

#include "codec2.h"

struct Codec2Config
{
    uint32_t    mode;
};

TEST(Codec2, Test)
{
    int mode = CODEC2_MODE_3200;
    struct CODEC2 *codec = codec2_create(mode);
    codec2_destroy(codec);
}

//  FIN
