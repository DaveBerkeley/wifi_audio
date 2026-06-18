
#include <stdlib.h>

#include "panglos/debug.h"
#include "panglos/object.h"

using namespace panglos;

#include "status.h"

    /*
     *  RTSP Client state machine sets the RGB LED
     */

bool Callback::to_rgb(State state, RGB *rgb)
{
    ASSERT(rgb);
    switch (state)
    {
        case RTSP_Session::INIT  :  return false;
        case RTSP_Session::READY :  rgb->b = 0x40;   break;
        case RTSP_Session::PLAY  :  rgb->g = 0x40;   break;
        case RTSP_Session::DEAD  :  break; // all off
        default : ASSERT(0);
    }

    return true;
}

void Callback::set_state(State state)
{
    RGB rgb = { 0 };
    if (to_rgb(state, & rgb))
    {
        leds->set(0, rgb.r, rgb.g, rgb.b);
        leds->send();
    }
}

bool Callback::on_idle()
{
    if (!i2s)
    {
        i2s = (I2S*) Objects::objects->get("i2s");
        if (i2s) PO_DEBUG("got i2s");
    }

    if (i2s && (state == RTSP_Session::PLAY))
    {
        // check that audio data is being rxd
        const int rx = i2s->rx_bytes();

        if (rx != xfer)
        {
            xfer = rx;
            Time::tick_t now = Time::get();
            if (Time::elapsed(t, period))
            {
                t = now;
                toggle = !toggle;
                set_state(toggle ? state : RTSP_Session::DEAD);
            }
        }
    }

    // polled in the main thread
    if (state == done) return false;

    PO_DEBUG("state=%d", state);
    done = state;
    set_state(state);
    return false; // keep in list
}

Callback::Callback(LedStrip *_leds)
:   leds(_leds),
    i2s(0),
    state(RTSP_Session::DEAD),
    done(RTSP_Session::DEAD),
    toggle(false),
    xfer(0)
{
    ASSERT(leds);
}

void Callback::on_state(State s)
{
    // Notify the state change from client/rtsp server threads
    state = s;
}

bool Callback::on_idle(void *arg, Event *, Event::Queue *)
{
    // Called in the main thread
    ASSERT(arg);
    Callback *cb = (Callback *) arg;
    return cb->on_idle();
}

//  FIN
