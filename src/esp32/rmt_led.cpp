
#include "panglos/debug.h"
#include "panglos/device.h"
#include "panglos/object.h"

#include "panglos/esp32/rmt_strip.h"

using namespace panglos;

#include "rmt_led.h"

bool leds_init(Device *dev, void *arg)
{
    PO_DEBUG("");
    ASSERT(arg);
    struct LedsDef *def= (struct LedsDef*) arg;

    RmtLedStrip *leds = RmtLedStrip::create(def->n, 24, (RmtLedStrip::Type) def->type);
    ASSERT(leds);
    const bool ok = leds->init(0, def->pin);
    leds->set_all(0x10, 0x10, 0x10);
    leds->send();
    dev->add(Objects::objects, leds);
    return ok;
}

//
