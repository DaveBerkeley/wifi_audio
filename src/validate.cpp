
#include <stdlib.h>

#include "panglos/debug.h"
#include "panglos/storage.h"

using namespace panglos;

#include "validate.h"

//  Move to panglos?

    /*
     *
     */

void get_params(Storage &db, const struct IntParam *params)
{
    for (const struct IntParam *p = params; p->name; p++)
    {
        int32_t v = 0;
        if (!db.get(p->name, & v)) continue;
        if (p->validate && !p->validate(v, p->name)) continue;

        ASSERT(p->value);
        *p->value = v;
        PO_DEBUG("%s.%s=%d", db.get_ns(), p->name, *p->value);
    }
}

    /*
     *
     */

bool validate_range(int32_t v, const char *name, int32_t lo, int32_t hi)
{
    if ((v >= lo) && (v <= hi)) return true;
    PO_WARNING("%s %d not in range %d .. %d", name, v, lo, hi);
    return false;
}

bool validate_set(int32_t v, const char *name, const int32_t *set, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        if (v == set[i]) return true;
    }
    PO_WARNING("%s %d not in set[%d]", name, v, (int) n);
    return false;
}

//  FIN
