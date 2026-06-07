
#pragma once

#include "i2s.h"

class WavSource : public AudioSource
{
    size_t size;
    size_t limit;
    FILE* file;
    void (*on_done)(void *);
    void *on_done_arg;

public:
    WavSource(size_t _limit=0);
    ~WavSource();

    bool open(const char *path);
    bool done();

    void set_on_done(void (*fn)(void *), void *arg);

    // implement AudioSource
    virtual size_t read(void *dest, size_t bytes, int) override;
    virtual size_t max_read_bytes() override;
};

class WavSink
{
    size_t written;
    FILE* file;

    bool write_header(int offset, uint32_t data);
public:
    WavSink();
    ~WavSink();

    bool open(const char *path);
    bool close();

    bool write(void *data, size_t bytes);
};

//  FIN
