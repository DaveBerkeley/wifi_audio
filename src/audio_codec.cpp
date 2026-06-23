
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

AudioCodec *AudioCodec::make_codec(const char *name)
{
    char buff[64];

    if (!name)
    {
        Storage db("app");
        size_t size = sizeof(buff);
        if (!db.get("codec", buff, & size))
        {
            PO_WARNING("codec not specified");
            return 0;
        }
        name = buff;
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
