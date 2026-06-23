
#include <stdint.h>
#include <string.h>

#include "panglos/debug.h"

#include "panglos/storage.h"

using namespace panglos;

#include "audio_codec.h"

AudioCodec::Register *AudioCodec::codecs;

    /*
     *  Build codec from config in Storage
     */

AudioCodec *AudioCodec::make_codec()
{
    Storage db("app");

    char name[64];
    size_t size = sizeof(name);
    if (!db.get("codec", name, & size))
    {
        PO_WARNING("codec not specified");
        return 0;
    }

    for (AudioCodec::Register *r = AudioCodec::codecs; r; r = r->next)
    {
        if (strcmp(name, r->name)) continue;
        return r->maker();
    }

    PO_WARNING("no codec '%s' found", name);
    return 0;
}

//  FIN
