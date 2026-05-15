
    /*
     *  Simulate Storage API
     */

#include <stdint.h>

#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/storage.h"

namespace panglos {

Storage::Storage(const char *ns, bool verbose)
{
    EXPECT_STREQ(ns, "verbose");
    IGNORE(verbose);
}

Storage::~Storage()
{
}

bool Storage::get(char const *key, int32_t *v)
{
    if (!strcmp(key, "one"))
    {
        *v = !*v;
        return true;
    }
    if (!strcmp(key, "two"))
    {
        *v = !*v;
        return true;
    }
    return false;
}

};

//  FIN
