
RTSP/RTP Streaming of I2S audio for the ESP32.
----

Still under development ..

Written using my 
[panglos](https://github.com/DaveBerkeley/panglos)
framework.

It runs RTSP and RTP servers on an ESP32-S3 target.
This code can also be tested and run on Linux as it is target agnostic.

I2S audio data can be received by the ESP32 and transmitted over WiFi.

I've added an Opus audio encoder to send compressed stereo 16-bit 48kHz audio over an RTS stream.

It handles multiple clients using my SocketServer class in panglos.

It uses my [CLI library](https://github.com/DaveBerkeley/cli) enabling a CLI on both
USB/UART and telnet interfaces, again allowing multiple clients.

The logging is my standard panglos logger.

* [RFC3550](https://datatracker.ietf.org/doc/rfc3550/) RTP: A Transport Protocol for Real-Time Applications
* [RFC2326](https://datatracker.ietf.org/doc/rfc2326/) RTSP: Real Time Streaming Protocol v 1.0
* [RFC7826](https://datatracker.ietf.org/doc/rfc7826/) RTSP: Real Time Streaming Protocol v 2.0

* [RFC6716](https://datatracker.ietf.org/doc/rfc6716/) Definition of the Opus Audio Codec 
* [RFC8251](https://datatracker.ietf.org/doc/rfc8251/) Updates to the Opus Audio Codec 
* [RFC7587](https://datatracker.ietf.org/doc/rfc7587/) RTP Payload Format for the Opus Speech and Audio Codec 

* [xiph/opus source](https://github.com/xiph/opus) on GitHub

