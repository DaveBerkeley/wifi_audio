
#include <stdarg.h>

#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/queue.h"
#include "panglos/mutex.h"
#include "panglos/time.h"
#include "panglos/thread.h"

#include "panglos/io.h"
#include "panglos/drivers/uart.h"

using namespace panglos;

    /*
     *
     */

extern "C" {

const LUT Severity_lut[] = {
    // Logging Severity
    {   "NONE", S_NONE, },
    {   "CRITICAL", S_CRITICAL, },
    {   "ERROR", S_ERROR, },
    {   "WARNING", S_WARNING, },
    {   "NOTICE", S_NOTICE, },
    {   "INFO", S_INFO, },
    {   "DEBUG", S_DEBUG, },
    { 0, 0 },
};

    /*
     *  TODO : Move to Panglos
     */

const char *lut(const LUT *codes, int err)
{
    for (const LUT *code = codes; code->text; code++)
    {
        if (code->code == err)
        {
            return code->text;
        }
    }

    return "unknown";
}

int rlut(const LUT *lut, const char *s)
{
    for (; lut->text; lut++)
    {
        if (!strcmp(lut->text, s))
        {
            return lut->code;
        }
    }
    return 0;
}

void Error_Handler()
{
    assert(0);
}

void po_log(Severity s, const char* fmt, ...)
{
    IGNORE(s);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

    /*
     *  Thread name for logging
     */

const char *get_task()
{
    Thread *thread = Thread::get_current();
    return thread ? thread->get_name() : "main";
}

uint32_t get_time(void)
{
    return 0;
}

}

//  FIN
