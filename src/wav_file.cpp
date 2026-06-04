
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "panglos/debug.h"

#include "wav_file.h"

// Wav files are little-endian, of course.

struct WavFileHeader {         
    char        RiffChunk[4];   // 'RIFF'
    uint32_t    ChunkSize;      // size of the riff chunk (should be always filesize - 8)
    char        FileFormat[4];  // 'WAVE'
    char        FormatChunk[4]; // 'fmt '
    uint32_t    FormatSize;     // size of Format structure (should be always 16 byte)
    uint16_t    PcmFlags;       // bit 1: Signed data, bit 2: Float data.. 
    uint16_t    Channels;       // samples per frame (example: one stereo frame consist from 2 samples)
    uint32_t    SampleRate;     // frames per second (example: 44100 stereo-frames are played back per seccond)   
    uint32_t    ByteRate;       // bytes per second (example: one second float32 stereo-track data: 44100frames * 2channels * 4bytes  )
    uint16_t    BlockAlign;     // byte per frame (example: each float32 stero frame is 8 byte in size - one float is 4byte - 2 channels are 2 floats, each 4byte)
    uint16_t    BitDepth;       // bits per sample (example: one float32 is 4 byte where each byte has 8 bit... so: 32 bit per sample )  */
    char        DataChunk[4];   // 'data'
    uint32_t    DataSize;       // size of of payload data (should be the total file size minus this headers size of 44 byte)
};

    /*
     *
     */

FILE* WavSource::wav_error(const char *text, int err)
{
    PO_ERROR("%s err=%d", text, err);
    return 0;
}

WavSource::WavSource(size_t _limit)
:   size(0),
    limit(_limit),
    file(0)
{ }

WavSource::~WavSource()
{
    fclose(file);
}

bool WavSource::open(const char *path)
{
    struct stat st;
    int err = stat(path, & st);
    if (err < 0)
        return wav_error("stat()", errno);

    size = st.st_size;
    PO_DEBUG("path=%s size=%d", path, (int) size);

    file = fopen(path, "rb");
    if (!file) return wav_error("fopen()", errno);

    struct WavFileHeader header;
    size_t n = fread(& header, sizeof(header), 1, file);
    if (n != 1) return wav_error("fread()", errno);

    if (memcmp(header.RiffChunk, "RIFF", 4))
        return wav_error("RIFF", 0);
    if (memcmp(header.FileFormat, "WAVE", 4))
        return wav_error("WAVE", 0);
    if (memcmp(header.FormatChunk, "fmt ", 4))
        return wav_error("fmt", 0);
    if (memcmp(header.DataChunk, "data", 4))
        return wav_error("data", 0);

    if (header.FormatSize != 16)
        return wav_error("FormatSize", 0);
    if (header.DataSize != (size - sizeof(header)))
        return wav_error("DataSize", 0);

    // we are only interested in pcm 16/4800/2 signals
    if (header.Channels != 2)
        return wav_error("Channels", 0);
    if (header.SampleRate != 48000)
        return wav_error("SampleRate", 0);
    if (header.ByteRate != (4 * 48000))
        return wav_error("ByteRate", 0);
    if (header.PcmFlags != 1)
        return wav_error("PcmFlags", 0);
    if (header.BitDepth != 16)
        return wav_error("BitDepth", 0);

    return file;
}

bool WavSource::done()
{
    return feof(file);
}

size_t WavSource::read(void *dest, size_t bytes, int)
{
    return fread(dest, 1, bytes, file);
}

size_t WavSource::max_read_bytes()
{
    return size;
}

//  FIN
