
#pragma once

    /*
     *  RTSP Response codes
     */

enum RTSP_ErrorCodes
{
    E_OK = 200,
    E_Bad_Request = 400,
    E_Unsupported_Media_Type = 415,
    E_Parameter_Not_Understood = 451,
    E_Not_Acceptable = 406,
    E_Method_Not_Valid_in_This_State = 455,
    E_Not_Implemented = 501,
    E_Service_Unavailable = 503,
    E_Version_Not_Supported = 505,
};

extern LUT response_lut[];

    /*
     *
     */

struct RtspHeader
{
    int cseq;
    int session_id;
    bool accept_sdp;

    struct IpAddr
    {
        uint8_t octets[4];
    };

    struct Transport
    {
        const char *transport;
        const char *mode;
        int ttl;
        int ssrc;
        bool rtp;
        bool unicast;
        bool rtcp_mux;
        int client_port[2];
        int server_port[2];
        int interleaved[2];
        int port[2];
        struct IpAddr destination;
        struct IpAddr source;
        struct IpAddr src_addr[2];
        struct IpAddr dest_addr[2];
    };

    static const int MAX_TRANSPORTS = 3;
    struct Transport transport[MAX_TRANSPORTS];
};

    /*
     *
     */

typedef enum 
{
    C_UNKNOWN = 0,
    C_OPTIONS,        // required
    C_DESCRIBE,       // recommended
    C_SETUP,          // required
    C_PAUSE,          // required
    C_PLAY,           // required
    //C_PLAY_NOTIFY,    // required S->C
    C_TEARDOWN,       // required
}   RtspCommand;

extern const LUT cmd_lut[];

    /*
     *
     */

class RTSP_Session
{
public:
    enum State
    {
        INIT,
        READY,
        PLAY,
    };

    class Handler
    {
    public:
        virtual ~Handler() { }
        virtual RtspCommand error(int code) = 0;
        virtual int get_last_error() = 0;        
        virtual int command(RtspCommand cmd, const char *uri, RtspHeader *hdrs, int error_code) = 0;
    };

    virtual ~RTSP_Session() {}

    virtual RtspCommand process(char *data, size_t s) = 0;
    virtual enum State get_state() = 0;

    static RTSP_Session *create(Handler *h);
};

//  FIN
