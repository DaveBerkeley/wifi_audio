
#include "gtest/gtest.h"

#include "panglos/storage.h"

using namespace panglos;

#include "validate.h"

TEST(Validate, Range)
{
    bool ok;

    for (int32_t i = -5; i < 10; i++)
    {
        ok = validate_range(i, "test", -5, 10);
        EXPECT_TRUE(ok);
    }
    ok = validate_range(-6, "test", -5, 10);
    EXPECT_FALSE(ok);
    ok = validate_range(11, "test", -5, 10);
    EXPECT_FALSE(ok);
}

TEST(Validate, Set)
{
    bool ok;

    const int32_t set[] = {
        0, 4, 10, -1,
    };

    ok = validate_set(0, "test", set, 4);
    EXPECT_TRUE(ok);
    ok = validate_set(4, "test", set, 4);
    EXPECT_TRUE(ok);
    ok = validate_set(10, "test", set, 4);
    EXPECT_TRUE(ok);
    ok = validate_set(-1, "test", set, 4);
    EXPECT_TRUE(ok);
    ok = validate_set(-2, "test", set, 4);
    EXPECT_FALSE(ok);
    ok = validate_set(1, "test", set, 4);
    EXPECT_FALSE(ok);
    ok = validate_set(2, "test", set, 4);
    EXPECT_FALSE(ok);
    ok = validate_set(3, "test", set, 4);
    EXPECT_FALSE(ok);
    ok = validate_set(5, "test", set, 4);
    EXPECT_FALSE(ok);
}

//  FIN
