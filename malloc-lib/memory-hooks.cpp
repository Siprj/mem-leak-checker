#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
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
#include "data-chunk-storage.h"
#include "data-chunk-types.h"


extern "C" {


// Pointers to function from libc (this calls are "replaced" by our functions).
// This is initialized by initMemLeakChecker.
void * (*libcCalloc)(size_t nmemb, size_t size);
void * (*libcMalloc)(size_t size);
void   (*libcFree)(void *ptr);
void * (*libcRealloc)(void *ptr, size_t size);
void * (*libcMemalign)(size_t blocksize, size_t bytes);


// Static variables counting number of calls each memory operation functions.
std::atomic<int> numberOfMalloc(0);
std::atomic<int> numberOfFree(0);
std::atomic<int> numberOfCalloc(0);
std::atomic<int> numberOfRealloc(0);
std::atomic<int> numberOfMemalign(0);


enum InitStates{
    NOT_INIT = 0,
    INIT_IN_PROGRESS = 1,
    FIRST_BACKTRACE_IN_PROGRESS = 2,
    INIT_DONE = 3
};
std::atomic<int>initsState(0);

// Only for initializatin porpuses.
std::recursive_mutex initRecursiveMutex;


// This function initialize and reinitialize whole memory leak checker library.
// Attribute ((constructor)) was removed because it was unreliable.
static void initMemLeakChecker();
static void  __attribute__((destructor)) deinitMemLeakChecker();


// Initialize memory leak checker.
// This function is not sometimes called by starting program so we need to
// check if init is done by memory hooks!!!
static void initMemLeakChecker()
{
    initRecursiveMutex.lock();
    initsState.store(InitStates::INIT_IN_PROGRESS);
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

    initsState.store(InitStates::FIRST_BACKTRACE_IN_PROGRESS);

    // First backtrace will call malloc so we need to call it here to prevent
    // recursive call of malloc. After that the logging in malloc can be
    // switched on.
    void *buffer[BACK_TRACE_LENGTH];
    backtrace(buffer, BACK_TRACE_LENGTH);

    // Initialize data storage
    DataChunStorage::initDataChunkStorage();

    initsState.store(InitStates::INIT_DONE);
    initRecursiveMutex.unlock();
}


// Destructor function is called when module is unloaded.
// It finish reports and clean resources.
static void  __attribute__((destructor)) deinitMemLeakChecker()
{
    DataChunStorage::storeSummary(numberOfMalloc.load(), numberOfFree.load(),
                                  numberOfCalloc.load(), numberOfRealloc.load(),
                                  numberOfMemalign.load());
    DataChunStorage::deinitDataChunkStorage();
}


void *malloc(size_t size)
{
    // This atomic operation is here only for "better" performance.
    // We don't want malloc call to serialize calls with mutex.
    if(initsState.load(std::memory_order_acq_rel) != InitStates::INIT_DONE)
    {
        // This lock is recursive to ensure no deadlock while initializing.
        initRecursiveMutex.lock();
        if(initsState.load() == InitStates::NOT_INIT)
        {
            initMemLeakChecker();
        }
        // If this function is call from the same thread and is time of 
        // initialization, we should return NULL because pointer to original 
        // malloc is not set.
        if(initsState.load(std::memory_order_acq_rel) == InitStates::INIT_IN_PROGRESS)
        {
            // Clean after function.
            initRecursiveMutex.unlock();
            return NULL;

        }
        if(initsState.load(std::memory_order_acq_rel) == InitStates::FIRST_BACKTRACE_IN_PROGRESS)
        {
            initRecursiveMutex.unlock();
            return libcMalloc(size);
        }
        initRecursiveMutex.unlock();
    }

    // Use malloc function from libc library (standard malloc) and store address
    // of new allocated memory.
    void *ptr = libcMalloc(size);

    // Increment number of malloc calls (can be relaxed because nothing else
    // depend on it).
    numberOfMalloc.fetch_add(1, std::memory_order_relaxed);

    DataChunkMalloc dataChunk;
    dataChunk.typeNumberId = CHUNK_TYPE_ID_MALLOC;
    dataChunk.addressOfNewMemory = ptr;
    dataChunk.memorySize = size;

    // Generate backtrace report.
    backtrace(dataChunk.backTrace, BACK_TRACE_LENGTH);

    DataChunStorage::storeDataChunk((void*)&dataChunk);

    return ptr;
}


void free(void *ptr)
{
    // This atomic operation is here only for "better" performance.
    // We don't want malloc call to serialize calls with mutex.
    if(initsState.load(std::memory_order_acq_rel) != InitStates::INIT_DONE)
    {
        // This lock is recursive to ensure no deadlock while initializing.
        initRecursiveMutex.lock();
        if(initsState.load() == InitStates::NOT_INIT)
        {
            initMemLeakChecker();
        }
        initRecursiveMutex.unlock();
    }


    // Increment number of free calls (can be relaxed because nothing else
    // depend on it).
    numberOfFree.fetch_add(1, std::memory_order_relaxed);
    libcFree(ptr);

    DataChunkFree dataChunk;
    dataChunk.typeNumberId = CHUNK_TYPE_ID_FREE;
    dataChunk.addressOfNewMemory = ptr;
    DataChunStorage::storeDataChunk(&dataChunk);
}


void *realloc(void *ptr, size_t size)
{
    // This atomic operation is here only for "better" performance.
    // We don't want malloc call to serialize calls with mutex.
    if(initsState.load(std::memory_order_acq_rel) != InitStates::INIT_DONE)
    {
        // This lock is recursive to ensure no deadlock while initializing.
        initRecursiveMutex.lock();
        if(initsState.load() == InitStates::NOT_INIT)
        {
            initMemLeakChecker();
        }
        initRecursiveMutex.unlock();
    }


    // Increment number of realloc calls (can be relaxed because nothing else
    // depend on it).
    numberOfRealloc.fetch_add(1, std::memory_order_relaxed);
    void *nptr = libcRealloc(ptr, size);
    return nptr;
}


void *calloc(size_t nmemb, size_t size)
{
    // This atomic operation is here only for "better" performance.
    // We don't want malloc call to serialize calls with mutex.
    if(initsState.load(std::memory_order_acq_rel) != InitStates::INIT_DONE)
    {
        // This lock is recursive to ensure no deadlock while initializing.
        initRecursiveMutex.lock();
        if(initsState.load() == InitStates::NOT_INIT)
        {
            initMemLeakChecker();
        }
        // If this function is call from the same thread and is time of 
        // initialization, we should return NULL because pointer to original 
        // calloc is not set.
        if(initsState.load(std::memory_order_acq_rel) == InitStates::INIT_IN_PROGRESS)
        {
            // clean after function
            initRecursiveMutex.unlock();
            return NULL;

        }
        initRecursiveMutex.unlock();
    }


    // Increment number of calloc calls (can be relaxed because nothing else
    // depend on it).
    numberOfCalloc.fetch_add(1, std::memory_order_relaxed);
    void *ptr = libcCalloc(nmemb, size);
    return ptr;

}


void *memalign(size_t blocksize, size_t bytes)
{
    // This atomic operation is here only for "better" performance.
    // We don't want malloc call to serialize calls with mutex.
    if(initsState.load(std::memory_order_acq_rel) != InitStates::INIT_DONE)
    {
        // This lock is recursive to ensure no deadlock while initializing.
        initRecursiveMutex.lock();
        if(initsState.load() == InitStates::NOT_INIT)
        {
            initMemLeakChecker();
        }
        initRecursiveMutex.unlock();
    }

    // Increment number of memalign calls (can be relaxed because nothing else
    // depend on it).
    numberOfMemalign.fetch_add(1, std::memory_order_relaxed);
    void *ptr = libcMemalign(blocksize, bytes);
    return ptr;
}


}
