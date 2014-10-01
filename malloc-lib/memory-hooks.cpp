#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <execinfo.h>
#include <sys/types.h>
#include <stdint.h>
#include <atomic>
#include <mutex>
#include <assert.h>

// include configuration
#include "config.h"

#include "../common-headers/common-enums.h"


extern "C" {


// Pointers to function from libc (this calls are "replaced" by our functions).
// This is initialized by initMemLeakChecker.
static void * (*libcCalloc)(size_t nmemb, size_t size);
static void * (*libcMalloc)(size_t size);
static void   (*libcFree)(void *ptr);
static void * (*libcRealloc)(void *ptr, size_t size);
static void * (*libcMemalign)(size_t blocksize, size_t bytes);


// Static variables counting number of calls each memory operation functions.
std::atomic<int> numberOfMalloc(0);
std::atomic<int> numberOfFree(0);
std::atomic<int> numberOfCalloc(0);
std::atomic<int> numberOfRealloc(0);
std::atomic<int> numberOfMemalign(0);


enum InitStates{
    NOT_INIT = 0,
    INIT_IN_PROGRESS = 1,
    INIT_DONE = 2
};
std::atomic<int>initsState(0);

// Only for initializatin porpuses.
std::recursive_mutex initRecursiveMutex;

// Flag which lock store chung function to make it thread safe
// Mutex cant be used becouse it need to use -pthread
// -pthread flag make so library (only DL_PRELOAD make this) do not execute
// init functions
std::atomic_flag storeChunkLock = ATOMIC_FLAG_INIT;


typedef struct {
    u_int8_t typeNumberId;              // Type of data chunk.
    u_int8_t data[];
}DataChunkBase;

typedef struct __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    void* addressOfNewMemory;           // Address of new memory.
    u_int32_t memorySize;               // Size of allocated memory.
    void* backTrace[BACK_TRACE_LENGTH]; // Backtrace of last n calls.
} DataChunkMalloc;

typedef struct __attribute__ ((packed)) {
    u_int8_t typeNumberId;              // Type of data chunk.
    void* addressOfNewMemory;           // Address to be freed.
} DataChunkFree;

void storeDataChunk(void* dataChunk);
static void __attribute__((constructor)) initMemLeakChecker();
static void  __attribute__((destructor)) deinitMemLeakChecker();
void storeSumary();


// Initialize memory leak checker.
// This function is not sometimes called by starting program so we need to
// check if init is done in memory hooks!!!
static void __attribute__((constructor)) initMemLeakChecker()
{
    // Find all original memory functions.
    // No point in continuing if any memory function wasn't found.
    libcMalloc     = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
    assert(libcMalloc != NULL);
    libcFree       = (void (*)(void*))dlsym(RTLD_NEXT, "free");
    assert(libcFree != NULL);
    libcCalloc     = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
    assert(libcCalloc != NULL);
    libcRealloc    = (void* (*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
    assert(libcRealloc != NULL);
    libcMemalign   = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "memalign");
    assert(libcMemalign != NULL);

    // Truncate binary trace file and write header.
    // Header are 4 bytes and it store size of pointer (size can chane by HW
    // architecture).
    int logFileFd;
    logFileFd = open(LOG_FILE, O_WRONLY | O_CREAT | O_TRUNC ,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    u_int8_t sizeOfPointer = sizeof(void*);
    u_int8_t backtraceLength = BACK_TRACE_LENGTH;
    write(logFileFd, &sizeOfPointer, sizeof(u_int8_t));
    write(logFileFd, &backtraceLength, sizeof(u_int8_t));

    // Close the logging file for future use (this will flush the data)
    close(logFileFd);


    // First backtrace will call malloc so we need to call it here to prevent
    // recursive call of malloc. After that the logging in malloc can be
    // switched on.
    void *buffer[BACK_TRACE_LENGTH];
    backtrace(buffer, BACK_TRACE_LENGTH);
}


// Store data chunk generated by memory hooks
void storeDataChunk(void* dataChunk)
{
    while (storeChunkLock.test_and_set()) {} // lock function

    int logFileFd;
    logFileFd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND ,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    int writeSize;

    switch((int)((DataChunkBase*)dataChunk)->typeNumberId)
    {
    case CHUNK_TYPE_ID_MALLOC:
        writeSize = sizeof(DataChunkMalloc);
        break;
    case CHUNK_TYPE_ID_FREE:
        writeSize = sizeof(DataChunkFree);
        break;
    case CHUNK_TYPE_ID_CALLOC:
        writeSize = 0;  // Set to zero for now.
        break;
    case CHUNK_TYPE_ID_REALLOC:
        writeSize = 0;  // Set to zero for now.
        break;
    case CHUNK_TYPE_ID_MEMALIGN:
        writeSize = 0;  // Set to zero for now.
        break;
    default:
        writeSize = 0;
        break;
    }

    fprintf(stdout, "data type: %hhu, writeSize: %d\n", ((DataChunkBase*)dataChunk)->typeNumberId, writeSize);

    write(logFileFd, dataChunk, writeSize);

    close(logFileFd);

    storeChunkLock.clear(); // unlock function
}


// This function generate summary report and store it in dedicated file.
void storeSumary()
{
    // Open text logging file and truncate it (new summary will be written).
    int sumaryLogFileFd = open(LOG_FILE_SUMARY, O_WRONLY | O_CREAT | O_TRUNC ,
                               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    // Generate new summary message.
    // This method was chosen because there is no memory allocation.
    char string[200];
    sprintf(string, "malloc: %d\nfree: %d\ncalloc: %d\nrealloc: %d\n"
            "memalign: %d\n",
            numberOfMalloc.load(std::memory_order_release),
            numberOfFree.load(std::memory_order_release),
            numberOfCalloc.load(std::memory_order_relaxed),
            numberOfRealloc.load(std::memory_order_relaxed),
            numberOfMemalign.load(std::memory_order_relaxed));
    // Write message in to file
    write(sumaryLogFileFd, &string, strlen(string));
    // Close logging file
    close(sumaryLogFileFd);
}


// Destructor function is called when module is unloaded.
// It finish reports and clean resources.
static void  __attribute__((destructor)) deinitMemLeakChecker()
{
    storeSumary();
}


void *malloc(size_t size)
{

    // This atomic operation is here only for "beter" performace
    // We don't want to serialize malloc call with mutex
    if(initsState.load(std::memory_order_acq_rel) != InitStates::INIT_DONE)
    {
        // this lock is recursive to ensure that no dead lock ocure
        // while initializing
        initRecursiveMutex.lock();
        if(initsState.load() == InitStates::NOT_INIT)
        {
            initsState.store(InitStates::INIT_IN_PROGRESS);
            initMemLeakChecker();
            initsState.store(InitStates::INIT_DONE);
        }
        // If this function is call from the same thread is time of
        // initialization we whould return NULL becouse pointer to original
        // mallco is not set
        if(initsState.load(std::memory_order_acq_rel) == InitStates::INIT_IN_PROGRESS)
        {
            // clean after function
            initRecursiveMutex.unlock();
            return NULL;

        }
        initRecursiveMutex.unlock();
    }

    // Use malloc function from libc library (standard malloc) and store address
    // of new allocated memory.
    void *ptr = libcMalloc(size);

    // Check if the logging of malloc is running.
    // This operation can be most probably relaxed but must be checked!!!
    // TODO: Check memory order of this if!!!!!

    // Increment number of malloc calls (can by relaxed because nothing else
    // depend on it).
    numberOfMalloc.fetch_add(1, std::memory_order_relaxed);

    DataChunkMalloc dataChunk;
    dataChunk.typeNumberId = CHUNK_TYPE_ID_MALLOC;
    dataChunk.addressOfNewMemory = ptr;
    dataChunk.memorySize = size;

    // Don't know if we care about return value
    //        int numberOfBacktraces;
    /*numberOfBacktraces = */backtrace(dataChunk.backTrace, BACK_TRACE_LENGTH);

    storeDataChunk((void*)&dataChunk);

    return ptr;
}


void free(void *ptr)
{
    // This atomic operation is here only for "beter" performace
    // We don't want to serialize malloc call with mutex
    if(initsState.load(std::memory_order_acq_rel) != InitStates::INIT_DONE)
    {
        // this lock is recursive to ensure that no dead lock ocure
        // while initializing
        initRecursiveMutex.lock();
        if(initsState.load() == InitStates::NOT_INIT)
        {
            initsState.store(InitStates::INIT_IN_PROGRESS);
            initMemLeakChecker();
            initsState.store(InitStates::INIT_DONE);
        }
        initRecursiveMutex.unlock();
    }


    // Increment number of free calls (can by relaxed because nothing else
    // depend on it).
    numberOfFree.fetch_add(1, std::memory_order_relaxed);
    libcFree(ptr);

    DataChunkFree dataChunk;
    dataChunk.typeNumberId = CHUNK_TYPE_ID_FREE;
    dataChunk.addressOfNewMemory = ptr;
    storeDataChunk(&dataChunk);
}


void *realloc(void *ptr, size_t size)
{
    // This atomic operation is here only for "beter" performace
    // We don't want to serialize malloc call with mutex
    if(initsState.load(std::memory_order_acq_rel) != InitStates::INIT_DONE)
    {
        // this lock is recursive to ensure that no dead lock ocure
        // while initializing
        initRecursiveMutex.lock();
        if(initsState.load() == InitStates::NOT_INIT)
        {
            initsState.store(InitStates::INIT_IN_PROGRESS);
            initMemLeakChecker();
            initsState.store(InitStates::INIT_DONE);
        }
        initRecursiveMutex.unlock();
    }


    // Increment number of realloc calls (can by relaxed because nothing else
    // depend on it).
    numberOfRealloc.fetch_add(1, std::memory_order_relaxed);
    void *nptr = libcRealloc(ptr, size);
    return nptr;
}


void *calloc(size_t nmemb, size_t size)
{
    // This atomic operation is here only for "beter" performace
    // We don't want to serialize malloc call with mutex
    if(initsState.load(std::memory_order_acq_rel) != InitStates::INIT_DONE)
    {
        // this lock is recursive to ensure that no dead lock ocure
        // while initializing
        initRecursiveMutex.lock();
        if(initsState.load() == InitStates::NOT_INIT)
        {
            initsState.store(InitStates::INIT_IN_PROGRESS);
            initMemLeakChecker();
            initsState.store(InitStates::INIT_DONE);
        }
        // If this function is call from the same thread is time of
        // initialization we whould return NULL becouse pointer to original
        // mallco is not set
        if(initsState.load(std::memory_order_acq_rel) == InitStates::INIT_IN_PROGRESS)
        {
            // clean after function
            initRecursiveMutex.unlock();
            return NULL;

        }
        initRecursiveMutex.unlock();
    }


    // Increment number of calloc calls (can by relaxed because nothing else
    // depend on it).
    numberOfCalloc.fetch_add(1, std::memory_order_relaxed);
    void *ptr = libcCalloc(nmemb, size);
    return ptr;

}


void *memalign(size_t blocksize, size_t bytes)
{
    // This atomic operation is here only for "beter" performace
    // We don't want to serialize malloc call with mutex
    if(initsState.load(std::memory_order_acq_rel) != InitStates::INIT_DONE)
    {
        // this lock is recursive to ensure that no dead lock ocure
        // while initializing
        initRecursiveMutex.lock();
        if(initsState.load() == InitStates::NOT_INIT)
        {
            initsState.store(InitStates::INIT_IN_PROGRESS);
            initMemLeakChecker();
            initsState.store(InitStates::INIT_DONE);
        }
        initRecursiveMutex.unlock();
    }

    // Increment number of memalign calls (can by relaxed because nothing else
    // depend on it).
    numberOfMemalign.fetch_add(1, std::memory_order_relaxed);
    void *ptr = libcMemalign(blocksize, bytes);
    return ptr;
}


}