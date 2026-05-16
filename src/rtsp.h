
class RTSP_Session
{
public:
    class Handler
    {
    public:
        virtual void response(const char *data, size_t s) = 0;
    };

    virtual ~RTSP_Session() {}

    virtual void process(char *data, size_t s) = 0;

    static RTSP_Session *create(Handler *h);
};

//  FIN
