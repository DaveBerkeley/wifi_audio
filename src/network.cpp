
#include <stdlib.h>
#include <string.h>

#include "panglos/debug.h"

#include "cli/src/cli.h"

#include "panglos/object.h"
#include "panglos/storage.h"
#include "panglos/network.h"

#include "panglos/app/cli.h"

using namespace panglos;

    /*
     *
     */

static void cli_net(CLI *cli, CliCommand *)
{
    Network *net = (Network*) Objects::objects->get("net");

    Network::Iterator iter;
    Interface *iface = net->get_interface(& iter);

    if (!iface)
    {
        cli_print(cli, "no interfaces%s", cli->eol);
        return;
    }

    while (iface)
    {
        Interface::State state;

        if (iface->is_connected(& state))
        {
            char ip[32];
            char gw[32];
            char mask[32];
            state.ip.tostr(ip, sizeof(ip));
            state.gw.tostr(gw, sizeof(gw));
            state.mask.tostr(mask, sizeof(mask));
            cli_print(cli, "%s ip=%s mask=%s gw=%s %s", iface->get_name(), ip, mask, gw, cli->eol);
        }
        else
        {
            cli_print(cli, "%s not connected%s", iface->get_name(), cli->eol);
        }
        iface = net->get_interface(& iter);
    }
}

static void cli_net_ap(CLI *cli, CliCommand *)
{
    Network *net = (Network*) Objects::objects->get("net");
    ASSERT(net);

    Network::Iterator iter;
    Interface *iface = net->get_interface(& iter);

    if (!iface)
    {
        cli_print(cli, "no interfaces%s", cli->eol);
        return;
    }

    int idx = 0;
    const char *ssid = cli_get_arg(cli, idx++);

    if (!ssid)
    {
        // List APs
        while (iface)
        {
            WiFiInterface *wifi = iface->get_wifi();
            if (wifi)
            {
                struct WiFiInterface::ApIter iter;
                AccessPoint *ap = wifi->get_ap(& iter);

                while (ap)
                {
                    cli_print(cli, "%s %s %s", iface->get_name(), ap->ssid, cli->eol);
                    ap = wifi->get_ap(& iter);
                }
            }

            iface = net->get_interface(& iter);
        }
        return;
    }

    if (!strcmp(ssid, "del"))
    {
        ssid = cli_get_arg(cli, idx++);
        if (!ssid)
        {
            cli_print(cli, "no password specified%s", cli->eol);
            return;
        }
        while (iface)
        {
            WiFiInterface *wifi = iface->get_wifi();
            if (wifi)
            {
                wifi->del_ap(ssid);
                cli_print(cli, "%s deleted from iface=%s%s", ssid, iface->get_name(), cli->eol);
                return;
            }
            iface = net->get_interface(& iter);
        }
        return;
    }

    const char *pw = cli_get_arg(cli, idx++);
    if (!pw)
    {
        cli_print(cli, "no password specified%s", cli->eol);
        return;
    }

    while (iface)
    {
        WiFiInterface *wifi = iface->get_wifi();
        if (wifi)
        {
            wifi->add_ap(ssid, pw);
            cli_print(cli, "%s added to iface=%s%s", ssid, iface->get_name(), cli->eol);
            return;
        }
        iface = net->get_interface(& iter);
    }

    cli_print(cli, "unable to add AP to interface(s)%s", cli->eol);
}

static Interface *get_iface(CLI *cli)
{
    int idx = 0;
    const char *s = cli_get_arg(cli, idx++);

    Network *net = (Network*) Objects::objects->get("net");
    ASSERT(net);
    Interface *iface = net->get_interface(s);

    if (!iface)
    {
        cli_print(cli, "no interface found%s", cli->eol);
    }

    return iface;
}

static void cli_net_stop(CLI *cli, CliCommand *)
{
    Interface *iface = get_iface(cli);
    if (!iface) { return; }

    cli_print(cli, "disconnecting %s %s", iface->get_name(), cli->eol);
    iface->disconnect();
}

static void cli_net_start(CLI *cli, CliCommand *)
{
    Interface *iface = get_iface(cli);
    if (!iface) { return; }

    cli_print(cli, "connecting %s...%s", iface->get_name(), cli->eol);
    iface->connect();
}

static CliCommand cli_net_start_cmd = { .cmd="start", .handler=cli_net_start, .help="start [iface]" };
static CliCommand cli_net_stop_cmd = { .cmd="stop", .handler=cli_net_stop, .help="stop [iface]", .next = & cli_net_start_cmd };
static CliCommand cli_net_ap_cmd = { .cmd="ap", .handler=cli_net_ap, .help="|del ssid|ssid pw", .next = & cli_net_stop_cmd };
static CliCommand cli_net_cmd = { .cmd="net", .handler=cli_net, .help="list ifaces", .subcommand = & cli_net_ap_cmd };

    /*
     *
     */

static bool add_ap(WiFiInterface *iface, const char *_ssid, const char *_pw)
{
    Storage db("wifi");

    char ssid[32];
    size_t s = sizeof(ssid);
    if (!db.get(_ssid, ssid, & s))
    {
        PO_ERROR("no ssid defined");
        return false;
    }

    char pw[32];
    s = sizeof(pw);
    if (!db.get(_pw, pw, & s))
    {
        PO_ERROR("no pw defined");
        return false;
    }

    iface->add_ap(ssid, pw);

    return true;
}

    /*
     *
     */

void start_network()
{
    PO_DEBUG("");

    Network *network = new Network;
    WiFiInterface *iface = WiFiInterface::create();
    network->add_interface(iface);

    Objects::objects->add("net", network);

    const char *postfix[] = { "", "2", "3", "4", 0 };
    for (const char **pf = postfix; *pf; pf++)
    {
        char ssid[32];
        char pw[32];
        snprintf(ssid, sizeof(ssid), "ssid%s", *pf);
        snprintf(pw, sizeof(pw), "pw%s", *pf);
        add_ap(iface, ssid, pw);
    }

    iface->connect();

    add_cli_command(& cli_net_cmd);
}

//   FIN
