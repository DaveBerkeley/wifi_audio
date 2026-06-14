
#pragma once

namespace panglos
{
    class Storage;
};

struct IntParam
{
    const char *name;
    int32_t *value;
    bool (*validate)(int32_t v, const char *name);
};

bool validate_range(int32_t v, const char *name, int32_t lo, int32_t hi);
bool validate_set(int32_t v, const char *name, const int32_t *set, size_t n);

void get_params(Storage &db, const struct IntParam *params);

//  FIN
