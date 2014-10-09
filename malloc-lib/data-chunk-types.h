#ifndef DATACHUNKTYPES_H
#define DATACHUNKTYPES_H

#include <stdint.h>

typedef struct __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    void* addressOfNewMemory;           // Address of new memory.
    u_int64_t memorySize;               // Size of allocated memory.
    void* backTrace; // Backtrace of last n calls.
} DataChunkMalloc;

typedef struct __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    void* addressOfNewMemory;           // Address to be freed.
} DataChunkFree;


typedef union __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    DataChunkFree freeChunk;
    DataChunkMalloc mallocChunk;
}DataChunkBase;


#endif // DATACHUNKTYPES_H
