
#pragma once

class AudioCodec;
class Allocator;
class AudioSource;

struct ServerDesc
{
    const char *ip;
    int rtsp_port;
    int rtp_ports[2];
    AudioCodec *codec;
    Allocator *allocator;
    AudioSource *audio_source;
};

void _server(void *);

void run_server_thread(struct ServerDesc *);

struct CLI;
void add_rtp_commands(struct CLI *cli);

//  FIN
