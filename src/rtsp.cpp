
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

const LUT cmd_plus_lut[] = {
    { "KILL",    C_KILL, },
    { 0, 0 },
};

static bool unsupported(const char *cmd)
{
    // return true if the command is in the list of valid but unsupported commands
    static const char *cmds[] = {
        "ANNOUNCE",
        "GET_PARAMETER",
        "PLAY_NOTIFY",
        "RECORD",
        "REDIRECT",
        "SET_PARAMETER",
        0,
    };
    for (const char **s = cmds; *s; s++)
    {
        if (!strcasecmp(*s, cmd))
            return true;
    }
    return false;
}

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
    {   "Unsupported Media Type", E_Unsupported_Media_Type },
    {   "Parameter Not Understood", E_Parameter_Not_Understood },
    {   "Not Enough Bandwidth", 453 },
    {   "Session Not Found", 454 },
    {   "Method Not Valid in This State", E_Method_Not_Valid_in_This_State },
    {   "Header Field Not Valid for Resource", 456 },
    {   "Invalid Range", 457 },
    {   "arameter Is Read-Only", 458 },
    {   "Aggregate Operation Not Allowed", 459 },
    {   "Only Aggregate Operation Allowed", 460 },
    {   "Unsupported Transport", E_Unsupported_Transport },
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

    bool get_number(int *result, char *line, char **_end=0, int base=10)
    {
        if (!line) return false;
        char *end = 0;
        long long val = strtoll(line, & end, base);
        *result = (int) val;
        if (_end) *_end = end;
        return true;
    }

    bool get_ipaddr(struct RtspHeader::IpAddr *addr, char *s, char **_end=0)
    {
        for (int i = 0; i < 4; i++)
        {
            int value = 0;
            char *end = 0;
            if (!get_number(& value, s, & end))
                return false;
            addr->octets[i] = (uint8_t) value;
            if (_end) *_end = end;
        }
        return true;
    }

    bool parse_transport_option(struct RtspHeader::Transport *hdr, char *option)
    {
        PO_DEBUG("'%s'", option);

        struct Binaries {
            const char *cmd;
            bool set;
            bool *flag;
        };

        const struct Binaries bins[] = {
            { "unicast", true, & hdr->unicast },
            { "multicast", false, & hdr->unicast },
            { "rtcp-mux", true, & hdr->rtcp_mux },
            { 0 },
        };

        for (const struct Binaries *b = bins; b->cmd; b++)
        {
            if (!strcmp(b->cmd, option))
            {
                *b->flag = b->set;
                return true;
            }
        }

        struct Range {
            const char *cmd;
            int *values;
        };

        const struct Range ranges[] = {
            { "client_port=", hdr->client_port },
            { "server_port=", hdr->server_port },
            { "interleaved=", hdr->interleaved },
            { "port=", hdr->port },
            { 0 },
        };

        for (const struct Range *r = ranges; r->cmd; r++)
        {
            if (strncmp(r->cmd, option, strlen(r->cmd)))
                continue;
            option = & option[strlen(r->cmd)];

            char *end = 0;
            if (!get_number(& r->values[0], option, & end))
                return false;
            if (end && (end[0] != '-'))
                return false;
            option = end + 1;
            end = 0;
            if (!get_number(& r->values[1], option, & end))
                return false;

            PO_DEBUG("%s %d,%d", r->cmd, r->values[0], r->values[1]);
            return true;
        }

        struct TextOption
        {
            const char *cmd;
            const char **value;
        };

        const struct TextOption texts[] = {
            { "mode=", & hdr->mode },
            { 0 },
        };

        for (const struct TextOption *t = texts; t->cmd; t++)
        {
            if (strncmp(t->cmd, option, strlen(t->cmd)))
                continue;
            option = & option[strlen(t->cmd)];
            * t->value = option;
            return true;
        }

        struct NumberOption
        {
            const char *cmd;
            int *value;
        };

        const struct NumberOption numbers[] = {
            { "ttl=", & hdr->ttl },
            { "ssrc=", & hdr->ttl },
            { 0 },
        };

        for (const struct NumberOption *n = numbers; n->cmd; n++)
        {
            if (strncmp(n->cmd, option, strlen(n->cmd)))
                continue;
            option = & option[strlen(n->cmd)];
            char *end = 0;
            if (!get_number(n->value, option, & end, 0))
                return false;
            return true;
        }

        struct IpAddr
        {
            const char *cmd;
            RtspHeader::IpAddr *addr;
        };

        const struct IpAddr ips[] = {
            { "destination=", & hdr->destination },
            { "source=", & hdr->source },
            { 0 },
        };

        for (const struct IpAddr *i = ips; i->cmd; i++)
        {
            if (strncmp(i->cmd, option, strlen(i->cmd)))
                continue;
            option = & option[strlen(i->cmd)];
            if (!get_ipaddr(i->addr, option))
                return false;
            return true;
        }

        const struct IpAddr iprange[] = {
            {   "src_addr=", & hdr->src_addr[0] },
            {   "dest_addr=", & hdr->dest_addr[0] },
            { 0 },
        };

        for (const struct IpAddr *i = iprange; i->cmd; i++)
        {
            if (strncmp(i->cmd, option, strlen(i->cmd)))
                continue;
            option = & option[strlen(i->cmd)];
            char *end = 0;
            if (!get_ipaddr(i->addr, option, & end))
                return false;
            if (!(end && end[0] == ','))
                return false;
            option = end + 1;
            if (!get_ipaddr(i->addr, option, & end))
                return false;
            return true;
        }

        PO_DEBUG("not handled");
        return false;
    }

    bool parse_transport(struct RtspHeader *hdr, char **save)
    {
        // "Transport:" may have more than one transport spec.
        char *lines[RtspHeader::MAX_TRANSPORTS] = { 0 };

        char *ss = *save;
        int idx = 0;
        size_t sz = strlen(ss) - 4;
        for (size_t i = 0; i < sz; i++)
        {
            if (strncmp(& ss[i], "RTP/", 4))
                continue;

            lines[idx] = & ss[i];
            // remove any trailing ", " from the preceeding line
            if (idx)
            {
                char *prev = lines[idx-1];
                for (size_t si = lines[idx] - prev - 1; si > 0; si--)
                {
                    if ((prev[si] == ' ') || (prev[si] == ','))
                        prev[si] = '\0';
                    else
                        break;
                }
            }
            idx += 1;
            if (idx >= RtspHeader::MAX_TRANSPORTS)
                break;
        }

        for (int idx = 0; idx < RtspHeader::MAX_TRANSPORTS; idx++)
        {
            if (!lines[idx]) break;
            PO_DEBUG("parse transport spec.");

            // set defaults
            hdr->transport[idx].transport = "UDP";
            hdr->transport[idx].mode = "PLAY";
            hdr->transport[idx].unicast = false;

            char *l_save = 0;
            char *s = strtok_r(lines[idx], ";", & l_save);
            // parse the RTP/AVP/UDP transport-protocol/profile/lower-transport part
            char *t_save = 0;
            char *protocol = strtok_r(s, "/", & t_save);
            if (protocol && !strcmp("RTP", protocol))
                hdr->transport[idx].rtp = true;
            char *profile = strtok_r(0, "/", & t_save);
            char *transport = strtok_r(0, "/", & t_save);
            if (transport) hdr->transport[idx].transport = transport;

            PO_DEBUG("'%s/%s/%s'", protocol, profile, hdr->transport[idx].transport);

            // read the parameters
            while (!!(s = strtok_r(0, ";", & l_save)))
            {
                if (!parse_transport_option(& hdr->transport[idx], s))
                    return false;
            }
        }

        return true;
    }

    bool read_headers(LineParser *lp, struct RtspHeader *hdr)
    {
        // Set header defaults
        hdr->accept_sdp = true;

        while (char *line = lp->getline())
        {
            char *save = 0;
            char *header = strtok_r(line, " ", & save);
            if (!header) break;
            PO_DEBUG("%s", header);

            if (!strncasecmp(header, "CSeq:", 5))
            {
                char *s = strtok_r(0, " ", & save);
                if (!get_number(& hdr->cseq, s))
                    return false;
            }
            else if (!strncasecmp(header, "Session:", 8))
            {
                char *s = strtok_r(0, " ", & save);
                if (!get_number(& hdr->session_id, s))
                    return false;
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
            else if (!strncasecmp(header, "Transport:", 10))
            {
                if (!parse_transport(hdr, & save))
                    return false;
            }
            else
            {
                PO_DEBUG("no parser for '%s'", header);
            }
        }
        return true;
    }

    RtspCommand parse(LineParser *lp, const char *uri, RtspCommand cmd, int error_code)
    {
        ASSERT(lp);

        RtspHeader headers = { 0 };
        read_headers(lp, & headers);

        int code = handler->command(cmd, uri, & headers, error_code);
        return ((code == E_OK) && (error_code == E_OK)) ? cmd : C_UNKNOWN;
    } 

    static bool is_allowable(const RtspCommand *allowable, RtspCommand cmd)
    {
        // Check if the command is permitted in this state
        for (int i = 0; allowable[i] != C_UNKNOWN; i++)
        {
            if (allowable[i] == cmd) 
                return true;
        }
        return false;
    }

    RtspCommand parse(const RtspCommand *allowable, char *data, size_t sz)
    {
        int code = E_OK;

        if (!sz) return handler->error(E_Bad_Request);
        ASSERT(data);

        LineParser lp(data, sz);

        char *line = lp.getline();
        if (!line) return handler->error(E_Bad_Request);

        char *save = 0;
        char *cmd = strtok_r(line, " ", & save);
        if (!cmd) handler->error(E_Bad_Request);
        PO_DEBUG("%s", cmd);
        RtspCommand rtsp_cmd = (RtspCommand) rlut(cmd_lut, cmd);

        if (rtsp_cmd == C_UNKNOWN)
        {
            rtsp_cmd = (RtspCommand) rlut(cmd_plus_lut, cmd);
        }

        if (rtsp_cmd == C_UNKNOWN) {
            PO_INFO("Unknown command '%s'", cmd);
            if (unsupported(cmd))
                code = E_Not_Implemented;
            else
                return handler->error(E_Bad_Request);
        }

        PO_DEBUG("%s", cmd);

        // Check if the command is permitted in this state
        if (code == E_OK)
            if (!is_allowable(allowable, rtsp_cmd))
                code = E_Method_Not_Valid_in_This_State;

        char *uri = strtok_r(0, " ", & save);
        if (!uri) code = E_Bad_Request;

        // Check it is version 1.0 (I'm only supporting 1.0)
        char *version = strtok_r(0, " ", & save);
        if (!version) code = E_Bad_Request;
        if (strcmp(version, "RTSP/1.0")) code = E_Version_Not_Supported;

        switch(rtsp_cmd)
        {
            case C_KILL : return parse(& lp, uri, rtsp_cmd, E_OK);
            // valid commands
            case C_PAUSE :
            case C_TEARDOWN :
            case C_PLAY :
            case C_DESCRIBE :
            case C_SETUP :
            case C_OPTIONS : return parse(& lp, uri, rtsp_cmd, code);
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

    enum State state;

    virtual enum State get_state() override
    {
        return state;
    }

    void set_state(enum State s)
    {
        state = s;

        static const LUT states[] = {
            {   "INIT", INIT,   },
            {   "READY", READY,   },
            {   "PLAY", PLAY,   },
            {   "DEAD", DEAD,   },
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

        switch (state)
        {
            case INIT : return init;
            case READY : return ready;
            case PLAY : return ready; // same commands as READY
            default : ASSERT(0);
        }
        return init;
    }

    virtual RtspCommand process(char *data, size_t s) override
    {
        Parser parser(handler);
        RtspCommand cmd = parser.parse(allowable(), data, s);

        if (cmd == C_KILL)
        {
            set_state(DEAD);
            return C_KILL;
        }

        if (cmd == C_TEARDOWN)
        {
            set_state(DEAD);
            return C_UNKNOWN;
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
            case DEAD :
            {
                return C_UNKNOWN;
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
