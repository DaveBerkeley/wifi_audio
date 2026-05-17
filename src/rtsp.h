
    /*
     *  RTSP Response codes
     */

enum RTSP_ErrorCodes
{
    E_OK = 200,
    E_Bad_Request = 400,
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

struct RtspHeaders
{
    int cseq;
    bool accept_sdp;
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

    /*
     *
     */

class RTSP_Session
{
public:
    class Handler
    {
    public:
        virtual RtspCommand error(int code) = 0;
        virtual int get_last_error() = 0;        
        virtual int describe(const char *uri, RtspHeaders *hdrs) = 0;
        virtual int options(const char *uri, RtspHeaders *hdrs) = 0;
    };

    virtual ~RTSP_Session() {}

    virtual RtspCommand process(char *data, size_t s) = 0;

    static RTSP_Session *create(Handler *h);
};

//  FIN
