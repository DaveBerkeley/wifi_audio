
#pragma once

#include "i2s.h"

class WavSource : public AudioSource
{
    size_t size;
    size_t limit;
    FILE* file;

    static FILE* wav_error(const char *text, int err);

public:
    WavSource(size_t _limit=0);
    ~WavSource();

    bool open(const char *path);
    bool done();

    // implement AudioSource
    virtual size_t read(void *dest, size_t bytes, int) override;
    virtual size_t max_read_bytes() override;
};

//  FIN
