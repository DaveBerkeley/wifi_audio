
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

static FILE* error(const char *text, int err)
{
    PO_ERROR("%s err=%d", text, err);
    return 0;
}

WavSource::WavSource(size_t _limit)
:   size(0),
    limit(_limit),
    file(0),
    on_done(0),
    on_done_arg(0)
{ }

WavSource::~WavSource()
{
    fclose(file);
}

void WavSource::set_on_done(void (*fn)(void *), void *arg)
{
    on_done = fn;
    on_done_arg = arg;
}

bool WavSource::open(const char *path)
{
    struct stat st;
    int err = stat(path, & st);
    if (err < 0)
        return error("stat()", errno);

    size = st.st_size;
    PO_DEBUG("path=%s size=%d", path, (int) size);

    file = fopen(path, "rb");
    if (!file) return error("fopen()", errno);

    struct WavFileHeader header;
    size_t n = fread(& header, sizeof(header), 1, file);
    if (n != 1) return error("fread()", errno);

    if (memcmp(header.RiffChunk, "RIFF", 4))
        return error("RIFF", 0);
    if (memcmp(header.FileFormat, "WAVE", 4))
        return error("WAVE", 0);
    if (memcmp(header.FormatChunk, "fmt ", 4))
        return error("fmt", 0);
    if (memcmp(header.DataChunk, "data", 4))
        return error("data", 0);

    if (header.FormatSize != 16)
        return error("FormatSize", 0);
//    if (header.DataSize != (size - sizeof(header)))
//    {
//        PO_ERROR("header.DataSize=%#x expected=%#x", (int) header.DataSize, (int) (size - sizeof(header)));
//        return error("DataSize", 0);
//    }

    // we are only interested in pcm 16/4800/2 signals
    if (header.Channels != 2)
        return error("Channels", 0);
    if (header.SampleRate != 48000)
        return error("SampleRate", 0);
    if (header.ByteRate != (4 * 48000))
        return error("ByteRate", 0);
    if (header.PcmFlags != 1)
        return error("PcmFlags", 0);
    if (header.BitDepth != 16)
        return error("BitDepth", 0);

    return file;
}

bool WavSource::done()
{
    return feof(file);
}

size_t WavSource::read(void *dest, size_t bytes, int)
{
    if (on_done && done())
    {
        on_done(on_done_arg);
    }
    return fread(dest, 1, bytes, file);
}

size_t WavSource::max_read_bytes()
{
    return size;
}

    /*
     *
     */

WavSink::WavSink()
:   written(0),
    file(0)
{
}

WavSink::~WavSink()
{
    close();
}

bool WavSink::open(const char *path)
{
    file = fopen(path, "wb");
    if (!file) return error("fopen", errno);

    struct WavFileHeader header = { { 0 } };

    memcpy(header.RiffChunk,   "RIFF", 4);
    memcpy(header.FileFormat,  "WAVE", 4);
    memcpy(header.FormatChunk, "fmt ", 4);
    memcpy(header.DataChunk,   "data", 4);

    header.ChunkSize = 0; // fill this in later
    header.FormatSize = 16;
    header.PcmFlags = 1; // signed data
    header.Channels = 2;
    header.SampleRate = 48000;
    header.ByteRate = header.SampleRate * header.Channels * sizeof(int16_t);
    header.BlockAlign = 4;
    header.BitDepth = 16;
    header.DataSize = 0; // fill this in later

    size_t s = fwrite(& header, sizeof(header), 1, file);
    if (s != 1) return error("fwrite(header)", errno);

    return true;
}

bool WavSink::write_header(int offset, uint32_t data)
{
    int err = fseek(file, offset, SEEK_SET);
    if (err < 0) return error("fseek()", errno);
    size_t s = fwrite(& data, sizeof(data), 1, file);
    if (s != 1) return error("fwrite(data)", errno);
    return true;
}

bool WavSink::close()
{
    if (!file) return true;

    // Patch up the header fields now we know the total file length
    uint32_t ChunkSize = uint32_t(written + sizeof(struct WavFileHeader) - 8);
    uint32_t DataSize = uint32_t(written);
    struct WavFileHeader *header = 0;

    int err = fflush(file);
    if (err < 0) return error("fflush()", errno);

    if (!write_header((int)(intptr_t)(& header->ChunkSize), ChunkSize))
        return false;
    if (!write_header((int)(intptr_t)(& header->DataSize), DataSize))
        return false;

    err = fseek(file, 0, SEEK_END);
    if (err < 0) return error("fseek(END)", errno);

    err = fclose(file);
    if (err < 0) return error("fclose()", errno);
    file = 0;
    return true;
}

bool WavSink::write(void *data, size_t s)
{
    size_t err = fwrite(data, 1, s, file);
    if (err != s) return error("fwrite()", errno);
    written += err;
    return true;
}

//  FIN
