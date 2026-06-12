
#pragma once

#include "panglos/time.h"
#include "panglos/drivers/led_strip.h"
#include "panglos/app/event.h"

#include "i2s.h"
#include "rtsp.h"

    /*
     *  RTSP Client state machine sets the RGB LED
     */

class Callback : public RTSP_Status
{
    typedef RTSP_Session::State State;
    typedef struct panglos::LedStrip::RGB RGB;

    panglos::LedStrip *leds;
    State state;
    State done;
    bool toggle;
    I2S *i2s;
    int xfer;
    panglos::Time::tick_t t;
    const panglos::Time::tick_t period = 50;

    static bool to_rgb(State state, RGB *rgb);

    void set_state(State state);
    bool on_idle();

public:

    Callback(panglos::LedStrip *leds);

    virtual void on_state(State s) override;

    static bool on_idle(void *arg, panglos::Event *, panglos::Event::Queue *);
};

//  FIN
