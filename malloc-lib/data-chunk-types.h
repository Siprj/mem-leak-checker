#ifndef DATACHUNKTYPES_H
#define DATACHUNKTYPES_H

#include <stdint.h>

typedef struct __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    void* addressOfNewMemory;           // Address of new memory.
    u_int64_t memorySize;               // Size of allocated memory.
    void* backTrace;                    // Backtrace of last call.
} DataChunkMalloc;

typedef struct __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    void* addressOfNewMemory;           // Address to be freed.
} DataChunkFree;

typedef struct __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    void* addressOfNewMemory;           // Address of new memory.
    u_int32_t numberOfMembers;          // Number of members to allocate.
    u_int32_t sizeOfMember;             // Size of one member.
    void* backTrace;                    // Backtrace of last call.
} DataChunkCalloc;

typedef struct __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    void* addressOfNewMemory;           // Address of new memory.
    void* addressOfOldMemory;           // Address of olde memory which is
                                        // deleted.
    u_int64_t memorySize;               // Size of allocated memory.
    void* backTrace;                    // Backtrace of last call.
} DataChunkRealloc;

typedef struct __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    void* addressOfNewMemory;           // Address of new memory.
    u_int32_t memorySize;               // Size of new memory
    u_int32_t alignment;
    void* backTrace;                    // Backtrace of last call.
} DataChunkMemalign;


typedef union __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    DataChunkFree freeChunk;
    DataChunkMalloc mallocChunk;
    DataChunkCalloc callocChunk;
    DataChunkRealloc reallocChunk;
    DataChunkMemalign memalignChunk;
}DataChunkBase;


#endif // DATACHUNKTYPES_H
