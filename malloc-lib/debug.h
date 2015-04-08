#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>


void initDebug(uint32_t level);
void memoryDump(void *address, size_t size);

enum debug_levels
{
    DEBUG_LEVEL_NONE = 0,
    DEBUG_LEVEL_ERROR,
    DEBUG_LEVEL_WARN,
    DEBUG_LEVEL_INFO,
    DEBUG_LEVEL_DEBUG
};

#ifdef DEBUG_ENABLE

static const char *levels[] =
{
    "ERROR",
    "WARN ",
    "INFO ",
    "DEBUG"
};


bool checkDebugLevel(uint32_t level);


/**
 * Multi-platform way to get current time
 */
void current_utc_time(struct timespec *ts);


#define DEBUG_PRINT(level, msg, ...) \
    do { \
        if(checkDebugLevel(level)) { \
            struct timespec ts; \
            current_utc_time(&ts); \
            fprintf(stderr, "%.2ld:%.2ld:%.2ld.%.3ld [%ld] (%s:%d) %s - ", \
            ts.tv_sec / 3600 % 24, ts.tv_sec / 60 % 60, ts.tv_sec % 60,\
            ts.tv_nsec / 1000000, syscall(SYS_gettid), __FILE__ , \
            __LINE__, levels[level - 1]); \
            fprintf(stderr, msg, ##__VA_ARGS__); \
            fprintf(stderr, "\n"); } } \
    while(0)



#define ERROR(msg, ...) DEBUG_PRINT(DEBUG_LEVEL_ERROR, msg, ##__VA_ARGS__)
#define WARN(msg, ...)  DEBUG_PRINT(DEBUG_LEVEL_WARN,  msg, ##__VA_ARGS__)
#define INFO(msg, ...)  DEBUG_PRINT(DEBUG_LEVEL_INFO,  msg, ##__VA_ARGS__)
#define DEBUG(msg, ...) DEBUG_PRINT(DEBUG_LEVEL_DEBUG, msg, ##__VA_ARGS__)

#define DEBUG_INIT(level) initDebug(level)

#else

#define ERROR(msg, ...)
#define WARN(msg, ...)
#define INFO(msg, ...)
#define DEBUG(msg, ...)

#define DEBUG_INIT(level)

#endif
