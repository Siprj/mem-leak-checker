#ifndef DATACHUNKTYPES_H
#define DATACHUNKTYPES_H


#include "config.h"


typedef struct {
    u_int8_t typeNumberId;              // Type of data chunk.
    u_int8_t data[];
}DataChunkBase;

typedef struct __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    void* addressOfNewMemory;           // Address of new memory.
    u_int64_t memorySize;               // Size of allocated memory.
    void* backTrace[BACK_TRACE_LENGTH]; // Backtrace of last n calls.
} DataChunkMalloc;

typedef struct __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    void* addressOfNewMemory;           // Address to be freed.
} DataChunkFree;


#endif // DATACHUNKTYPES_H
