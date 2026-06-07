
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
            size(0),
            next(0)
        {
            payload = new uint8_t[s];
            memcpy(payload, p, s);
            size = s;
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

static struct OpusConfig config
{
    .bit_rate = 96000,
    .complexity = 8,
    .packet_rate = 60,
    .encode = true,
    .decode = true,
};

TEST(Opus, Test)
{
    AudioCodec *codec = AudioCodec::create(& config);

    int pt = codec->get_payload_type();
    EXPECT_EQ(97, pt);

    const char *name = codec->name();
    EXPECT_STREQ("Opus", name);

    WavSource wav;

    const char *ipath = "sine.wav";
    //const char *ipath = "all_night_long.wav";
    const char *opath = "/tmp/a.wav";

    const bool okay = wav.open(ipath);
    EXPECT_TRUE(okay);

    size_t read_samples = codec->samples_per_packet() * 2;
    size_t read_sz = read_samples * sizeof(int16_t);
    int16_t *read_buff = new int16_t[read_samples];
    size_t packet_sz = codec->max_payload_size();
    uint8_t *encode_buff = new uint8_t[packet_sz];

    Packets packets;

    PO_DEBUG("Encode from WAV file '%s'", ipath);

    panglos::Time::tick_t now = panglos::Time::get();
    const int period = 10000; // ms tick on Linux

    while (!wav.done())
    {
        size_t bytes = wav.read(read_buff, read_sz, 0);
        size_t samples = bytes / (2 * sizeof(int16_t));

        if (samples != codec->samples_per_packet())
            break; // must have complete set of samples for a packet

        size_t c = codec->encode(read_buff, samples, encode_buff, packet_sz);
        //PO_DEBUG("bytes=%d c=%d", (int) bytes, (int) c);
        packets.append(encode_buff, c);

        if (!panglos::Time::elapsed(now, period)) continue;
        now += period;
        PO_DEBUG(".");
    }

    WavSink sink;

    bool ok = sink.open(opath);
    EXPECT_TRUE(ok);

    PO_DEBUG("Decode to WAV file '%s'", opath);

    now = panglos::Time::get();
    while (true)
    {
        Packets::Packet *packet = packets.pop();
        if (!packet) break;

        size_t s = codec->decode(packet->payload, packet->size, read_buff, read_sz);
        EXPECT_TRUE(s > 0);
        bool ok = sink.write(read_buff, s * 2 * sizeof(int16_t));
        EXPECT_TRUE(ok);
        delete packet;

        if (!panglos::Time::elapsed(now, period)) continue;
        now += period;
        PO_DEBUG(".");
    }

    sink.close();

    delete[] read_buff;
    delete[] encode_buff;

    delete codec;
}

//  FIN
