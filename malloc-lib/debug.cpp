#include "debug.h"

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
