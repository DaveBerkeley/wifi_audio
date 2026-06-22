
#pragma once

#include "i2s.h"

class RawSource : public AudioSource
{
protected:
    size_t size;
    FILE* file;
    void (*on_done)(void *);
    void *on_done_arg;

public:
    RawSource();
    ~RawSource();

    virtual bool open(const char *path);
    bool done();

    void set_on_done(void (*fn)(void *), void *arg);

    // implement AudioSource
    virtual size_t read(void *dest, size_t bytes, int) override;
    virtual size_t max_read_bytes() override;
};

class WavSource : public RawSource
{
public:
    virtual bool open(const char *path) override;
};

    /*
     *
     */

class RawSink
{
protected:
    size_t written;
    FILE* file;

    virtual bool write_header(int offset, uint32_t data);
public:
    RawSink();
    virtual ~RawSink();

    virtual bool open(const char *path);
    virtual bool close();

    bool write(void *data, size_t bytes);
};

class WavSink : public RawSink
{
    virtual bool write_header(int offset, uint32_t data) override;
public:
    virtual bool open(const char *path) override;
    virtual bool close() override;
};

//  FIN
