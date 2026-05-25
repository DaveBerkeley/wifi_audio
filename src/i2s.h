
#pragma once

class AudioSource
{
public:
    virtual ~AudioSource() { }
    // blocking read
    virtual size_t read(void *dest, size_t bytes)  = 0;
};

class I2S : public AudioSource
{
public:
    static I2S *create(int sck, int ws, int sd, uint32_t freq, bool byte_swap);
};

//  FIN
