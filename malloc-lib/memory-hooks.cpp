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
    INIT_DONE = 2
};
std::atomic<int>initsState(0);

// Only for initializatin porpuses.
std::recursive_mutex initRecursiveMutex;


// This function initialize and reinitialize whole memory leak checker library.
// Attribute ((constructor)) was removed because it was unreliable.
static void initMemLeakChecker();
static void  __attribute__((destructor)) deinitMemLeakChecker();


// RecursiveCallCounter is thread specific variable which distinguish if the
// thread is calling memory hooks recursively. Some of internal implementation 
// may cause memory allocation (backtrace, etc.).
__thread int recursiveCallCounter = 0;


// Check if the memory hooks are called recursively  or if it is first call per
// thread. Must be called inside of all memory hooks!!!
// It must be used in pair with leavFunction().
// You could say it is acquire.
bool isFirstCall()
{
    return !recursiveCallCounter++;
}

// It is opposite of isFirstCall() function. It releases hook.
void leavFunction()
{
    recursiveCallCounter--;
}


// Initialize memory leak checker.
// This function is not sometimes called by starting program so we need to
// check if init is done by memory hooks!!!
static void initMemLeakChecker()
{
    isFirstCall();      // Don't care about return value. We know this is not
                        // called recursively. But must be called for future
                        // checks.
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

    // Init of libc function pointers is now complet and the rest will be
    // handled by isFirstCall() and leavFunction() functions.
    initsState.store(InitStates::INIT_DONE);

    // Initialize data storage
    DataChunStorage::initDataChunkStorage();
    leavFunction();
}


// Destructor function is called when module is unloaded.
// It finish reports and clean resources.
static void  __attribute__((destructor)) deinitMemLeakChecker()
{
    DataChunStorage::storeSummary(numberOfMalloc.load(std::memory_order_relaxed), numberOfFree.load(std::memory_order_relaxed),
                                  numberOfCalloc.load(std::memory_order_relaxed), numberOfRealloc.load(std::memory_order_relaxed),
                                  numberOfMemalign.load(std::memory_order_relaxed));
    DataChunStorage::deinitDataChunkStorage();
}


/*!
 * \brief isInitInProgress
 * \return return false if pointer to libc are not init yet.
 */
// TODO: Add std::memory_order flags
bool isInitInProgress()
{
    // This atomic operation is here only for "better" performance.
    // We don't want malloc call to serialize calls with mutex.
    // Serialization is caused only when init is in progress.
    if(initsState.load() != InitStates::INIT_DONE)
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
        if(initsState.load() == InitStates::INIT_IN_PROGRESS)
        {
            // Clean after function.
            initRecursiveMutex.unlock();
            return false;

        }
        initRecursiveMutex.unlock();
    }
    return true;
}


void *malloc(size_t size)
{
    if(!isInitInProgress())
    {
        return NULL;
    }

    // Use malloc function from libc library (standard malloc) and store address
    // of new allocated memory.
    void *ptr = libcMalloc(size);

    if(isFirstCall())
    {
        // Increment number of malloc calls (can be relaxed because nothing else
        // depend on it).
        numberOfMalloc.fetch_add(1, std::memory_order_relaxed);

        DataChunkBase dataChunk;
        dataChunk.typeNumberId = CHUNK_TYPE_ID_MALLOC;
        dataChunk.mallocChunk.addressOfNewMemory = ptr;
        dataChunk.mallocChunk.memorySize = size;
        dataChunk.mallocChunk.backTrace = __builtin_extract_return_addr(__builtin_return_address(0));

        DataChunStorage::storeDataChunk(&dataChunk);
    }

    leavFunction();
    return ptr;
}


void free(void *ptr)
{
    if(!isInitInProgress())
    {
        return;
    }

    libcFree(ptr);

    if(isFirstCall())
    {
        // Increment number of free calls (can be relaxed because nothing else
        // depend on it).
        numberOfFree.fetch_add(1, std::memory_order_relaxed);

        DataChunkBase dataChunk;
        dataChunk.typeNumberId = CHUNK_TYPE_ID_FREE;
        dataChunk.freeChunk.addressOfNewMemory = ptr;
        DataChunStorage::storeDataChunk(&dataChunk);
    }
    leavFunction();
}


void *realloc(void *ptr, size_t size)
{
    if(!isInitInProgress())
    {
        return NULL;
    }

    void *nptr = libcRealloc(ptr, size);

    if(isFirstCall())
    {
        // Increment number of realloc calls (can be relaxed because nothing else
        // depend on it).
        numberOfRealloc.fetch_add(1, std::memory_order_relaxed);
    }

    leavFunction();
    return nptr;
}


void *calloc(size_t nmemb, size_t size)
{
    if(!isInitInProgress())
    {
        return NULL;
    }

    void *ptr = libcCalloc(nmemb, size);

    if(isFirstCall())
    {
        // Increment number of calloc calls (can be relaxed because nothing else
        // depend on it).
        numberOfCalloc.fetch_add(1, std::memory_order_relaxed);
    }

    leavFunction();
    return ptr;

}


void *memalign(size_t blocksize, size_t bytes)
{
    if(!isInitInProgress())
    {
        return NULL;
    }

    void *ptr = libcMemalign(blocksize, bytes);

    if(isFirstCall())
    {
        // Increment number of memalign calls (can be relaxed because nothing else
        // depend on it).
        numberOfMemalign.fetch_add(1, std::memory_order_relaxed);
    }

    leavFunction();
    return ptr;
}


}   // end of extern "C"


void* operator new(size_t size)
{
    if(!isInitInProgress())
    {
        return NULL;
    }

    void *ptr = libcMalloc(size);;

    DataChunkBase dataChunk;
    dataChunk.typeNumberId = CHUNK_TYPE_ID_MALLOC;
    dataChunk.mallocChunk.addressOfNewMemory = ptr;
    dataChunk.mallocChunk.memorySize = size;
    dataChunk.mallocChunk.backTrace = __builtin_extract_return_addr(__builtin_return_address(0));

    DataChunStorage::storeDataChunk((void*)&dataChunk);

    return ptr;
}


void* operator new[](size_t size)
{
    if(!isInitInProgress())
    {
        return NULL;
    }

    void *ptr = libcMalloc(size);

    DataChunkBase dataChunk;
    dataChunk.typeNumberId = CHUNK_TYPE_ID_MALLOC;
    dataChunk.mallocChunk.addressOfNewMemory = ptr;
    dataChunk.mallocChunk.memorySize = size;
    dataChunk.mallocChunk.backTrace = __builtin_extract_return_addr(__builtin_return_address(0));

    DataChunStorage::storeDataChunk((void*)&dataChunk);

    return ptr;
}


void operator delete(void *ptr)
{
    if(!isInitInProgress())
    {
        return;
    }

    DataChunkBase dataChunk;
    dataChunk.typeNumberId = CHUNK_TYPE_ID_FREE;
    dataChunk.freeChunk.addressOfNewMemory = ptr;

    DataChunStorage::storeDataChunk(&dataChunk);

    libcFree(ptr);
}


void operator delete[](void *ptr)
{
    if(!isInitInProgress())
    {
        return;
    }

    libcFree(ptr);

    DataChunkBase dataChunk;
    dataChunk.typeNumberId = CHUNK_TYPE_ID_FREE;
    dataChunk.freeChunk.addressOfNewMemory = ptr;

    DataChunStorage::storeDataChunk(&dataChunk);
}
