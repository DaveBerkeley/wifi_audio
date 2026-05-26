
#pragma once

class AudioCodec;

struct ServerDesc
{
    const char *ip;
    int rtsp_port;
    int rtp_ports[2];
    AudioCodec *codec;
};

void _server(void *);
void run_server(struct ServerDesc *);

class CLI;
void add_rtp_commands(CLI *cli);

//  FIN
