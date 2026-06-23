
#include <stdio.h>

#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/time.h"
#include "panglos/storage.h"

using namespace panglos;

#include "i2s.h"
#include "audio_codec.h"
#include "wav_file.h"

    /*
     *
     */

class Packets
{
public:
    struct Packet
    {
        uint8_t *payload;
        size_t size;
        Packet *next;

        Packet(uint8_t *p, size_t s)
        :   payload(0),
            size(s),
            next(0)
        {
            payload = new uint8_t[s];
            memcpy(payload, p, s);
        }

        ~Packet()
        {
            delete[] payload;
        }
    };

    Packet *head;
    Packet *tail;

    Packets()
    :   head(0),
        tail(0)
    {
    }

    ~Packets()
    {
        while (true)
        {
            Packet *p = pop();
            if (!p) break;
            delete p;
        }
    }

    Packet *pop()
    {
        Packet *p = head;
        if (!p) return 0;
        head = p->next;
        if (!head)
            tail = 0;
        return p;
    }

    void append(uint8_t *payload, size_t size)
    {
        Packet *packet = new Packet(payload, size);

        if (head == 0)
            head = packet;
        if (tail)
            tail->next = packet;
        tail = packet;
    }
};

    /*
     *
     */

static void test_codec(AudioCodec *codec, RawSource *source, RawSink *sink)
{

    const char *name = codec->name();
    PO_DEBUG("codec=%s", name);

    size_t read_samples = codec->samples_per_packet() * codec->num_chans();
    size_t read_sz = read_samples * codec->sample_size();
    size_t samples = read_sz / (codec->num_chans() * codec->sample_size());
    //PO_DEBUG("read_sz=%d samples=%d", (int) read_sz, (int) samples);
    int8_t *read_buff = new int8_t[read_sz];
    size_t packet_sz = codec->max_payload_size();
    uint8_t *encode_buff = new uint8_t[packet_sz];
    //PO_DEBUG("samples=%d read_samples=%d read_bytes=%d compressed_bytes=%d",
    //        (int) samples, (int) read_samples, (int) read_sz, (int) packet_sz);

    Packets packets;

    panglos::Time::tick_t now = panglos::Time::get();
    const int period = 10000; // ms tick on Linux
    size_t total_rd = 0;
    size_t total_wr = 0;

    while (!source->done())
    {
        size_t bytes = source->read(read_buff, read_sz, 0);
        if (!bytes)
        {
            //PO_DEBUG("read completed");
            break;
        }
        if (bytes < read_sz)
        {
            // must have complete set of samples for a packet
            //PO_DEBUG("final bytes=%d sz=%d discarded", (int) bytes, (int) read_sz);
            break;
        }

        total_rd += bytes;
        // bytes must be a round number of samples        
        ASSERT(bytes == read_sz);
        EXPECT_EQ(bytes, samples * codec->num_chans() * codec->sample_size());

        size_t c = codec->encode((int16_t*)read_buff, samples, encode_buff, packet_sz);
        EXPECT_LE(c, packet_sz);
        packets.append(encode_buff, c);
        total_wr += c;

        if (!panglos::Time::elapsed(now, period)) continue;
        now += period;
        PO_DEBUG(".");
    }

    EXPECT_TRUE(total_rd);
    EXPECT_TRUE(total_wr);
    PO_DEBUG("rd=%d wr=%d ratio=%d", (int) total_rd, (int) total_wr, int(total_rd / total_wr));

    now = panglos::Time::get();
    while (true)
    {
        Packets::Packet *packet = packets.pop();
        if (!packet) break;

        size_t s = codec->decode(packet->payload, packet->size, (int16_t*) read_buff, read_sz);
        EXPECT_TRUE(s > 0);
        bool ok = sink->write(read_buff, s * codec->num_chans() * codec->sample_size());
        EXPECT_TRUE(ok);
        delete packet;

        if (!panglos::Time::elapsed(now, period)) continue;
        now += period;
        PO_DEBUG(".");
    }

    sink->close();

    delete[] read_buff;
    delete[] encode_buff;
}

    /*
     *
     */

static void hint(const char *path)
{
    if (strstr(path, ".wav"))
    {
        PO_DEBUG("play with : mplayer %s", path);
        return;
    }
    if (strstr(path, ".raw"))
    {
        PO_DEBUG("play with : aplay -f S16_LE %s", path);
        return;
    }
    PO_DEBUG("unknown file format %s", path);
}

static RawSource *audio_reader(const char *path)
{
    RawSource *src = 0;

    if (strstr(path, ".wav"))
    {
        src = new WavSource;
    }
    else if (strstr(path, ".raw"))
    {
        src = new RawSource;
    }
    
    if (!src) return 0;
    if (!src->open(path)) return 0;
    return src;
}

    /*
     *
     */

TEST(Codec, Opus)
{
    static struct OpusConfig config
    {
        .bit_rate = 96000,
        .complexity = 5,
        .packet_rate = 60,
        .fs = 48000,
        .chans = 2,
        .app = OpusConfig::OP_AUDIO,
        .encode = true,
        .decode = true,
    };

    AudioCodec *codec = AudioCodec::create(& config);

    EXPECT_EQ(codec->sample_rate(), 48000);
    
    //const char *ipath = "sine.wav";
    const char *ipath = "audio_files/test_audio.wav";
    const char *opath = "/tmp/opus.wav";

    PO_DEBUG("Encode from file '%s'", ipath);
    RawSource *src = audio_reader(ipath);

    WavSink sink;
    bool ok = sink.open(opath);
    EXPECT_TRUE(ok);

    test_codec(codec, src, & sink);
    hint(opath);
    delete codec;
    delete src;
}

    /*
     *
     */

#include "codec2/src/codec2.h"

TEST(Codec, Codec2)
{
    struct Codec2Config config =
    {
        //.mode = CODEC2_MODE_3200,
        .mode = CODEC2_MODE_2400,
        //.mode = CODEC2_MODE_1600,
        //.mode = CODEC2_MODE_1300,
        //.mode = CODEC2_MODE_700C,
        .fs = 8000, // only used in the SDP header
    };

    AudioCodec *codec = AudioCodec::create(& config);

    EXPECT_EQ(codec->sample_rate(), 8000);

    const char *ipath = "audio_files/build/test_audio.raw";
    const char *opath = "/tmp/codec2.raw";
    
    PO_DEBUG("Encode from file '%s'", ipath);
    RawSource *src = audio_reader(ipath);

    RawSink sink;
    bool ok = sink.open(opath);
    EXPECT_TRUE(ok);

    test_codec(codec, src, & sink);
    hint(opath);
    delete codec;
    delete src;
}

    /*
     *
     */

TEST(Codec, MakeOpus)
{
    {
        Storage db("app");
        db.set("codec", "opus");
    }

    Storage db("opus");

    struct Pair {
        const char *name;
        int32_t value;
    };

    const struct Pair pairs[] = {
        {   "bit_rate",    96000 },
        {   "complexity",  8 },
        {   "packet_rate", 20 },
        {   "fs",          48000 },
        {   "chans",       1 },
        {   "app",         1 },
        {   0   },
    };

    for (const struct Pair *pair = pairs; pair->name; pair++)
    {
        db.set(pair->name, pair->value);
    }

    AudioCodec *codec = AudioCodec::make_codec();

    EXPECT_EQ(codec->sample_rate(), 48000);
    EXPECT_STREQ("Opus", codec->name());
    EXPECT_EQ(1, codec->num_chans());

    db.clear_all();

    delete codec;
}

TEST(Codec, MakeCodec2)
{
    {
        Storage db("app");
        db.set("codec", "codec2");
    }

    Storage db("codec2");

    struct Pair {
        const char *name;
        int32_t value;
    };

    const struct Pair pairs[] = {
        {   "mode", CODEC2_MODE_1300 },
        {   0   },
    };

    for (const struct Pair *pair = pairs; pair->name; pair++)
    {
        db.set(pair->name, pair->value);
    }

    AudioCodec *codec = AudioCodec::make_codec();

    if (!codec)
    {
        PO_ERROR("codec2 support not enabled");
    }
    else
    {
        EXPECT_STREQ("codec2", codec->name());
        EXPECT_EQ(1, codec->num_chans());
    }

    db.clear_all();

    delete codec;
}

//  FIN
