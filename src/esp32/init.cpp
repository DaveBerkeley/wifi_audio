
#include "panglos/debug.h"

#include "panglos/esp32/gpio.h"
#include "panglos/device.h"
#include "panglos/object.h"

#include "panglos/app/devices.h"
#include "panglos/app/event.h"
#include "panglos/app/cli_server.h"

using namespace panglos;

#include "i2s.h"
#include "server.h"
#include "esp32/init.h"

static bool rtp_init(void *, Event *, Event::Queue *)
{
    PO_DEBUG("");
    static struct ServerDesc info = {
        .ip = "0.0.0.0",
        .rtsp_port = 554,
        .rtp_ports = { 6000, 6001, },
    };
    run_server(& info);

    CLI *cli = (CLI*) Objects::objects->get("cli");
    ASSERT(cli);
    add_rtp_commands(cli);
 
    //I2S *i2s = (I2S*) Objects::objects->get("i2s");
    //ASSERT(i2s);
    
    return false; // INIT handlers must return false so multiple handlers can be run
}

void board_init(int sck, int ws, int sd)
{
    PO_DEBUG("");

#if 1
    I2S *i2s = I2S::create(sck, ws, sd);
    ASSERT(i2s);
    Objects::objects->add("i2s", i2s);
#endif

    if (Objects::objects->get("net"))
    {
        EventHandler::add_handler(Event::INIT, net_cli_init, 0);
    }

    EventHandler::add_handler(Event::INIT, rtp_init, 0);    
}

//  FIN
