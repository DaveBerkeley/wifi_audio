
#include <stdio.h>

#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/time.h"

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
            //memset(payload, 0xaa, s);
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

static void test_codec(AudioCodec *codec, const char *opath)
{
    WavSource wav;

    const char *ipath = "sine.wav";
    //const char *ipath = "all_night_long.wav";

    const bool okay = wav.open(ipath);
    if (!okay)
    {
        PO_WARNING("Unable to run test, can't open '%s'", ipath);
        return;
    }
    EXPECT_TRUE(okay);

    const char *name = codec->name();
    PO_DEBUG("codec=%s", name);

    size_t read_samples = codec->samples_per_packet() * codec->num_chans();
    size_t read_sz = read_samples * codec->sample_size();
    int8_t *read_buff = new int8_t[read_sz];
    size_t packet_sz = codec->max_payload_size();
    uint8_t *encode_buff = new uint8_t[packet_sz];

    Packets packets;

    PO_DEBUG("Encode from WAV file '%s'", ipath);

    panglos::Time::tick_t now = panglos::Time::get();
    const int period = 10000; // ms tick on Linux
    size_t total_rd = 0;
    size_t total_wr = 0;

    while (!wav.done())
    {
        size_t bytes = wav.read(read_buff, read_sz, 0);
        if (!bytes)
            break;
        size_t samples = bytes / (codec->num_chans() * codec->sample_size());
        total_rd += bytes;

        if (samples != codec->samples_per_packet())
            break; // must have complete set of samples for a packet

        size_t c = codec->encode((int16_t*)read_buff, samples, encode_buff, packet_sz);
        //PO_DEBUG("bytes=%d c=%d", (int) bytes, (int) c);
        packets.append(encode_buff, c);
        total_wr += c;

        if (!panglos::Time::elapsed(now, period)) continue;
        now += period;
        PO_DEBUG(".");
    }

    EXPECT_TRUE(total_rd);
    EXPECT_TRUE(total_wr);
    PO_DEBUG("rd=%d wr=%d ratio=%d", (int) total_rd, (int) total_wr, int(total_rd / total_wr));

    WavSink sink;

    bool ok = sink.open(opath);
    EXPECT_TRUE(ok);

    PO_DEBUG("Decode to WAV file '%s'", opath);

    now = panglos::Time::get();
    while (true)
    {
        Packets::Packet *packet = packets.pop();
        if (!packet) break;

        size_t s = codec->decode(packet->payload, packet->size, (int16_t*) read_buff, read_sz);
        EXPECT_TRUE(s > 0);
        bool ok = sink.write(read_buff, s * codec->num_chans() * codec->sample_size());
        EXPECT_TRUE(ok);
        delete packet;

        if (!panglos::Time::elapsed(now, period)) continue;
        now += period;
        PO_DEBUG(".");
    }

    sink.close();

    delete[] read_buff;
    delete[] encode_buff;

    PO_DEBUG("Written: '%s'", opath);
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
    test_codec(codec, "/tmp/opus.wav");
    delete codec;
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
        .mode = CODEC2_MODE_1200,
        .fs = 8000,
    };
    AudioCodec *codec = AudioCodec::create(& config);
    test_codec(codec, "/tmp/codec2.wav");
    delete codec;
}

//  FIN
