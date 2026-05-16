
#define __STDC_WANT_LIB_EXT1__ 1

#include <stdlib.h>
#include <string.h>

#include "panglos/debug.h"

#include "rtsp.h"

    /*
     *
     */

class Service
{
    char *uri;
public:
    Service(const char *_uri)
    :   uri(strdup(_uri))
    {
    }

    ~Service()
    {
        free(uri);
    }

    const char *server() { return "Server: PhonyServer/1.1"; }
};

    /*
     *
     */

typedef enum rtsp_commands
{
    OPTIONS = 1,    // required
    DESCRIBE,       // recommended
    SETUP,          // required
    PAUSE,          // required
    PLAY,           // required
    //PLAY_NOTIFY,    // required S->C
    TEARDOWN,       // required
}   RtspCommand;

    /*
     *
     */

const LUT cmd_lut[] = {
    { "OPTIONS", OPTIONS, },
    { "DESCRIBE", DESCRIBE, },
    { "SETUP", SETUP, },
    { "PAUSE", PAUSE, },
    { "PLAY", PLAY, },
    { "TEARDOWN", TEARDOWN, },
    { 0, 0 },
};

    /*
     *
     */

class LineParser
{
    char *data;
    size_t size;
public:
    LineParser(char *_data, size_t s)
    :   data(_data),
        size(s)
    {
        while (size > 2)
        {
            // RFC7826 5. ignore leading CRLFs
            if (strncmp(data, "\r\n", 2))
                break;
            getline();
        }
    }

    char *getline()
    {
        if (!size) return 0;
        for (size_t i = 0; i < (size-2); i++)
        {
            if (!strncmp(& data[i], "\r\n", 2))
            {
                char *start = data;
                data[i] = '\0';
                data = & data[i + 2];
                size -= i + 2;
                return start;
            }
        }
        return 0;
    }
};

    /*
     *  RTSP Response codes
     */

enum ErrorCodes
{
    Bad_Request = 400,
    Not_Acceptable = 406,
    Not_Implemented = 501,
    Service_Unavailable = 503,
    Version_Not_Supported = 505,
};

static LUT response_lut[] = {
    {   "OK", 200 },
    {   "Continue", 100 },
    {   "Moved Permanently", 301 },
    {   "Found", 302 },
    {   "See Other", 303 },
    {   "Not Modified", 304 },
    {   "Use Proxy", 305 },
    {   "Bad Request", Bad_Request },
    {   "Unauthorized", 401 },
    {   "Payment Required", 402 },
    {   "Forbidden", 403 },
    {   "Not Found", 404 },
    {   "Method Not Allowed", 405 },
    {   "Not Acceptable", Not_Acceptable },
    {   "Proxy Authentication Required", 407 },
    {   "Request Timeout", 408 },
    {   "Gone", 410 },
    {   "Precondition Failed", 412 },
    {   "Request Message Body Too Large", 413 },
    {   "Request-URI Too Long", 414 },
    {   "Unsupported Media Type", 415 },
    {   "Parameter Not Understood", 451 },
    {   "Not Enough Bandwidth", 453 },
    {   "Session Not Found", 454 },
    {   "Method Not Valid in This State", 455 },
    {   "Header Field Not Valid for Resource", 456 },
    {   "Invalid Range", 457 },
    {   "arameter Is Read-Only", 458 },
    {   "Aggregate Operation Not Allowed", 459 },
    {   "Only Aggregate Operation Allowed", 460 },
    {   "Unsupported Transport", 461 },
    {   "Destination Unreachable", 462 },
    {   "Destination Prohibited", 463 },
    {   "Data Transport Not Ready Yet", 464 },
    {   "Notification Reason Unknown", 465 },
    {   "Key Management Error", 466 },
    {   "Connection Authorization Required", 470 },
    {   "Connection Credentials Not Accepted", 471 },
    {   "Failure to Establish Secure Connection", 472 },
    {   "Internal Server Error", 500 },
    {   "Not Implemented", Not_Implemented },
    {   "Bad Gateway", 502 },
    {   "Service Unavailable", Service_Unavailable },
    {   "Gateway Timeout", 504 },
    {   "RTSP Version Not Supported", Version_Not_Supported },
    {   "Option Not Supported", 551 },
    {   "Proxy Unavailable", 553 },
    { 0, 0 },
};

    /*
     *
     */

class Parser
{
public:
    struct Headers
    {
        int cseq;
        bool accept_sdp;
    };

    Parser() { }

    bool get_number(int *result, char *line)
    {
        if (!line) return false;
        char *end = 0;
        long long val = strtoll(line, & end, 10);
        *result = (int) val;
        return true;
    }

    void read_headers(LineParser *lp, struct Headers *hdr)
    {
        while (char *line = lp->getline())
        {
            char *save = 0;
            char *header = strtok_r(line, " ", & save);
            if (!header) break;
            PO_DEBUG("%s", header);

            if (!strcmp(header, "CSeq:"))
            {
                char *s = strtok_r(0, " ", & save);
                get_number(& hdr->cseq, s);
            }
            else if (!strcmp(header, "Accept:"))
            {
                hdr->accept_sdp = false;
                while (char *s = strtok_r(0, " ", & save))
                {
                    if (!strcmp(s, "application/sdp"))
                    {
                        hdr->accept_sdp = true;
                        break;
                    }
                }
            }

        }
    }

    int describe(LineParser *lp, const char *uri)
    {
        ASSERT(lp);
        PO_DEBUG("DESCRIBE '%s'", uri);

        Headers headers = { .accept_sdp = true };

        // read the headers
        read_headers(lp, & headers);

        PO_DEBUG("cseq=%d", headers.cseq);
        PO_DEBUG("accept_sdp=%d", headers.accept_sdp);

        if (!headers.accept_sdp) return Not_Acceptable;

        return code;
    }

    int parse(char *data, size_t s)
    {
        if (!s) return Not_Implemented;
        ASSERT(data);

        LineParser lp(data, s);

        char *line = lp.getline();
        if (!line) return Not_Implemented;

        char *save = 0;
        char *cmd = strtok_r(line, " ", & save);
        if (!cmd) return false;
        const int c = rlut(cmd_lut, cmd);
        if (c == 0) {
            PO_INFO("Unknown command '%s'", cmd);
            return Not_Implemented;
        }

        char *uri = strtok_r(0, " ", & save);
        if (!uri) return Service_Unavailable;

        // Check it is version 2.0
        char *version = strtok_r(0, " ", & save);
        if (!version) return Version_Not_Supported;
        if (strcmp(version, "RTSP/2.0")) return Version_Not_Supported;

        switch((RtspCommand) c)
        {
            case OPTIONS : PO_DEBUG("OPTIONS"); break;
            case DESCRIBE : return describe(& lp, uri);
            case SETUP : PO_DEBUG("SETUP"); break;
            case PAUSE : PO_DEBUG("PAUSE"); break;
            case PLAY : PO_DEBUG("PLAY"); break;
            case TEARDOWN : PO_DEBUG("TEARDOWN"); break;
            default : ASSERT(0); break;
        }

        PO_DEBUG("");
        return Bad_Request;
    }
};

    /*
     *
     */

class Session : public RTSP_Session
{
    RTSP_Session::Handler *handler;

    virtual void process(char *data, size_t s) override
    {
        Parser parser;

        int code = parser.parse(data, s);
        if (code != 200)
        {
            PO_ERROR("%s %d", lut(response_lut, code), code);
            // TODO : send response code
        }
        PO_DEBUG("");
    }

public:
    Session(RTSP_Session::Handler *h)
    :   handler(h)
    {
    }
};

    /*
     *
     */

RTSP_Session *RTSP_Session::create(RTSP_Session::Handler *h)
{
    return new Session(h);
}

//  FIN
