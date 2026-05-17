
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

const LUT cmd_lut[] = {
    { "OPTIONS",    C_OPTIONS, },
    { "DESCRIBE",   C_DESCRIBE, },
    { "SETUP",      C_SETUP, },
    { "PAUSE",      C_PAUSE, },
    { "PLAY",       C_PLAY, },
    { "TEARDOWN",   C_TEARDOWN, },
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
     *
     */

LUT response_lut[] = {
    {   "OK", E_OK },
    {   "Continue", 100 },
    {   "Moved Permanently", 301 },
    {   "Found", 302 },
    {   "See Other", 303 },
    {   "Not Modified", 304 },
    {   "Use Proxy", 305 },
    {   "Bad Request", E_Bad_Request },
    {   "Unauthorized", 401 },
    {   "Payment Required", 402 },
    {   "Forbidden", 403 },
    {   "Not Found", 404 },
    {   "Method Not Allowed", 405 },
    {   "Not Acceptable", E_Not_Acceptable },
    {   "Proxy Authentication Required", 407 },
    {   "Request Timeout", 408 },
    {   "Gone", 410 },
    {   "Precondition Failed", 412 },
    {   "Request Message Body Too Large", 413 },
    {   "Request-URI Too Long", 414 },
    {   "Unsupported Media Type", 415 },
    {   "Parameter Not Understood", E_Parameter_Not_Understood },
    {   "Not Enough Bandwidth", 453 },
    {   "Session Not Found", 454 },
    {   "Method Not Valid in This State", E_Method_Not_Valid_in_This_State },
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
    {   "Not Implemented", E_Not_Implemented },
    {   "Bad Gateway", 502 },
    {   "Service Unavailable", E_Service_Unavailable },
    {   "Gateway Timeout", 504 },
    {   "RTSP Version Not Supported", E_Version_Not_Supported },
    {   "Option Not Supported", 551 },
    {   "Proxy Unavailable", 553 },
    { 0, 0 },
};

    /*
     *
     */

class Parser
{
    RTSP_Session::Handler *handler;

public:
    Parser(RTSP_Session::Handler *h)
    :   handler(h)
    {
        ASSERT(handler);
    }

    bool get_number(int *result, char *line)
    {
        if (!line) return false;
        char *end = 0;
        long long val = strtoll(line, & end, 10);
        *result = (int) val;
        return true;
    }

    void read_headers(LineParser *lp, struct RtspHeaders *hdr)
    {
        while (char *line = lp->getline())
        {
            char *save = 0;
            char *header = strtok_r(line, " ", & save);
            if (!header) break;
            PO_DEBUG("%s", header);

            if (!strncasecmp(header, "CSeq:", 5))
            {
                char *s = strtok_r(0, " ", & save);
                get_number(& hdr->cseq, s);
            }
            else if (!strncasecmp(header, "Accept:", 7))
            {
                hdr->accept_sdp = false;
                while (char *s = strtok_r(0, " ", & save))
                {
                    if (!strncasecmp(s, "application/sdp", 15))
                    {
                        hdr->accept_sdp = true;
                        break;
                    }
                }
            }

        }
    }

    RtspCommand describe(LineParser *lp, const char *uri)
    {
        ASSERT(lp);

        RtspHeaders headers = { .accept_sdp = true };
        read_headers(lp, & headers);

        int code = handler->describe(uri, & headers);
        return (code == E_OK) ? C_DESCRIBE : C_UNKNOWN;
    }

    RtspCommand options(LineParser *lp, const char *uri)
    {
        ASSERT(lp);

        RtspHeaders headers = { 0 };
        read_headers(lp, & headers);

        int code = handler->describe(uri, & headers);
        return (code == E_OK) ? C_OPTIONS : C_UNKNOWN;
    }

    RtspCommand parse(const RtspCommand *allowable, char *data, size_t s)
    {
        UNUSED(allowable); // TODO
        if (!s) return handler->error(E_Not_Implemented);
        ASSERT(data);

        LineParser lp(data, s);

        char *line = lp.getline();
        if (!line) return handler->error(E_Not_Implemented);

        char *save = 0;
        char *cmd = strtok_r(line, " ", & save);
        if (!cmd) handler->error(E_Not_Implemented);
        const int c = rlut(cmd_lut, cmd);
        if (c == 0) {
            PO_INFO("Unknown command '%s'", cmd);
            return handler->error(E_Not_Implemented);
        }

        // Check if the command is permitted in this state
        bool found = false;
        for (int i = 0; allowable[i] != C_UNKNOWN; i++)
        {
            if (allowable[i] == c) found = true;
        }
        if (!found)
            return handler->error(E_Method_Not_Valid_in_This_State);

        char *uri = strtok_r(0, " ", & save);
        if (!uri) return handler->error(E_Bad_Request);

        // Check it is version 2.0
        char *version = strtok_r(0, " ", & save);
        if (!version) return handler->error(E_Version_Not_Supported);
        if (strcmp(version, "RTSP/2.0")) return handler->error(E_Version_Not_Supported);

        switch((RtspCommand) c)
        {
            case C_OPTIONS : return options(& lp, uri);
            case C_DESCRIBE : return describe(& lp, uri);
            case C_SETUP : PO_DEBUG("SETUP"); break;
            case C_PAUSE : PO_DEBUG("PAUSE"); break;
            case C_PLAY : PO_DEBUG("PLAY"); break;
            case C_TEARDOWN : PO_DEBUG("TEARDOWN"); break;
            default : ASSERT(0); break;
        }

        PO_DEBUG("");
        return handler->error(E_Bad_Request);
    }
};

    /*
     *
     */

class Session : public RTSP_Session
{
    RTSP_Session::Handler *handler;

    enum State
    {
        INIT,
        READY,
        PLAY,
    };

    enum State state;

    void set_state(enum State s)
    {
        state = s;

        static const LUT states[] = {
            {   "INIT", INIT,   },
            {   "READY", READY,   },
            {   "PLAY", PLAY,   },
            { 0, 0 },
        };

        PO_DEBUG("state:=%s", lut(states, state));
    }

    const RtspCommand *allowable()
    {
        static const RtspCommand init[] = {
            C_SETUP,
            C_OPTIONS,
            C_DESCRIBE,
            C_UNKNOWN, // terminate
        };
        static const RtspCommand ready[] = {
            C_TEARDOWN,
            C_SETUP,
            C_OPTIONS,
            C_DESCRIBE,
            C_PLAY,
            C_PAUSE,
            C_UNKNOWN, // terminate
        };
        /*
        static const RtspCommand play[] = {
            C_TEARDOWN,
            C_SETUP,
            C_OPTIONS,
            C_DESCRIBE,
            C_PLAY,
            C_PAUSE,
            C_UNKNOWN, // terminate
        };
        */

        switch (state)
        {
            case INIT : return init;
            case READY : return ready;
            case PLAY : return ready;
            default : ASSERT(0);
        }
        return init;
    }

    virtual RtspCommand process(char *data, size_t s) override
    {
        Parser parser(handler);
        RtspCommand cmd = parser.parse(allowable(), data, s);

        if (cmd == C_TEARDOWN)
        {
            // TODO : terminate the state machine
            ASSERT(0);
        }

        switch (state)
        {
            case INIT :
            {
                if (cmd == C_SETUP) set_state(READY);
                break;
            }
            case READY :
            {
                if (cmd == C_PLAY) set_state(PLAY);
                break;
            }
            case PLAY :
            {
                if (cmd == C_PAUSE) set_state(READY);
                break;
            }
            default : ASSERT(0);
        }

        return cmd;
    }

public:
    Session(RTSP_Session::Handler *h)
    :   handler(h)
    {
        set_state(INIT);
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
