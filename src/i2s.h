
#pragma once

class AudioSource
{
public:
    virtual ~AudioSource() { }
    // blocking read
    virtual size_t read(void *dest, size_t bytes, int idx)  = 0;
    virtual size_t max_read_bytes()  = 0;

    virtual int rx_errors() { return 0; }
    virtual int rx_bytes() { return 0; }
};

class I2S : public AudioSource
{
};

//  FIN
