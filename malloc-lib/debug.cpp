#include "debug.h"

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif


static  uint8_t g_current_debug_level = DEBUG_LEVEL_NONE;


void initDebug(uint32_t level)
{
    g_current_debug_level = level > DEBUG_LEVEL_DEBUG ? DEBUG_LEVEL_DEBUG :
                                                        level;
}


void memoryDump(void *address, size_t size)
{
    for(int i = 0; i < size; i++)
    {
        fprintf(stderr, "%02X", ((uint8_t*)address)[i]);
    }
}


bool checkDebugLevel(uint32_t level)
{
    if(level <= g_current_debug_level)
        return true;
    return false;
}

void current_utc_time(struct timespec *ts)
{
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_REALTIME, ts);
#endif
}