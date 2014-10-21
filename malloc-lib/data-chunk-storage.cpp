#include "data-chunk-storage.h"

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <stdint.h>
#include <atomic>
#include <assert.h>

#include "../common-headers/common-enums.h"
#include "data-chunk-types.h"

#include "original-memory-fnc-pointers.h"


// Output paths string are not std::strings because of malloc
char *outputFilePath;
char *outputSummaryFilePath;


// File descriptor which represent binary logging file.
static int logFileFd;


// Flag which lock store chung function to make it thread safe
std::atomic_flag storeChunkLock = ATOMIC_FLAG_INIT;

// "Atomic" cache for data chunks. The atomicity should speed up whole process
// of saving data.
DataChunkBase dataCache[NUMBER_OF_CACHES][CACHE_SIZE];
// Index to cache. Indicate which indexes was already acquired by any writer.
std::atomic<int> indexStoreing[NUMBER_OF_CACHES];
// Index to cache. Indicate which indexes was already written by any writer.
std::atomic<int> indexStored[NUMBER_OF_CACHES];
// This variable distinguish between caches.
std::atomic<int> cacheIndex;

namespace DataChunStorage {
    // Function wich take number of cache and write it in to file. Also second
    // parameter is top boundary of cache (number of entries which will be
    // written).
    void writeCache(int cacheNumber, int numberOfEntries);
}


// Initialize data chunk storage.
// Find if the output path is presented inside at environment variables
void DataChunStorage::initDataChunkStorage()
{
    //Search through environment variables to find out output path. 
    //If output path is found, merge it with output file names.
    char *path = getenv(ENV_OUTPUT_PATH);
    if(path != NULL)
    {
        // Compute size of full path and add 10 to make sure it will have
        // enough space.
        int pathSize = strlen(path) + strlen(LOG_FILE) + 10;
        outputFilePath = (char*)libcMalloc(pathSize);

        assert(outputFilePath != NULL);

        strcat(outputFilePath, path);
        strcat(outputFilePath, "/");
        strcat(outputFilePath, LOG_FILE);

        // Set full output summary file name.
        pathSize = strlen(path) + strlen(LOG_FILE_SUMARY) + 10;
        outputSummaryFilePath = (char*)libcMalloc(pathSize);

        assert(outputSummaryFilePath != NULL);

        strcat(outputSummaryFilePath, path);
        strcat(outputSummaryFilePath, "/");
        strcat(outputSummaryFilePath, LOG_FILE_SUMARY);
    }
    else    // No environment variable found.
    {
        // Set default output file name.
        int pathSize = strlen(LOG_FILE) + 1;
        outputFilePath = (char*)libcMalloc(pathSize);
        assert(outputFilePath != NULL);
        strcat(outputFilePath, LOG_FILE);

        // Set default output summary file name.
        pathSize = strlen(LOG_FILE_SUMARY) + 1;
        outputSummaryFilePath = (char*)libcMalloc(pathSize);
        strcat(outputSummaryFilePath, LOG_FILE_SUMARY);
    }
    printf("output: %s\n", outputFilePath);
    printf("output-sumary: %s\n", outputSummaryFilePath);

    // Check if summary log file can be created.
    int testFile = open(outputSummaryFilePath, O_WRONLY | O_CREAT | O_TRUNC,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(testFile == -1)
    {
        printf("Can't open/create %s file!!!", outputSummaryFilePath);
        exit(1);
    }
    close(testFile);


    // Truncate binary trace file and write header. Header are 4 bytes and 
    // it store size of pointer (size can be dependent on HW architecture).
    logFileFd = open(outputFilePath, O_WRONLY | O_CREAT | O_TRUNC ,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(logFileFd == -1)
    {
        printf("Can't open/create %s file!!!", outputFilePath);
        exit(1);
    }
    u_int8_t sizeOfPointer = sizeof(void*);
    int i = write(logFileFd, &sizeOfPointer, sizeof(u_int8_t));
    (void)i;
    fsync(logFileFd);


    // Init cache variables
    for(int i = 0; i < NUMBER_OF_CACHES; i++)
    {
        indexStoreing[i] = 0;
        indexStored[i] = 0;
    }
    cacheIndex = 0;
}


void DataChunStorage::deinitDataChunkStorage()
{
    // Write rest of data in to file
    for(int i = 0; i < NUMBER_OF_CACHES; i++)
    {
        if(indexStored[i].load() > 0)
        {
            writeCache(i,indexStored[i].load()-1);
        }
    }
    close(logFileFd);
}


void DataChunStorage::writeCache(int cacheNumber, int numberOfEntries)
{
    // Lock until all data are written
    while (storeChunkLock.test_and_set()) {}

    // Go through cache and identify all data chunks. Write them with the
    // correct size.
    for(int i = 0; i < numberOfEntries; i++)
    {
        int writeSize;
        switch((int)dataCache[cacheNumber][i].typeNumberId)
        {
        case CHUNK_TYPE_ID_MALLOC:
        writeSize = sizeof(DataChunkMalloc);
        break;
        case CHUNK_TYPE_ID_FREE:
        writeSize = sizeof(DataChunkFree);
        break;
        case CHUNK_TYPE_ID_CALLOC:
        writeSize = 0; // Set to zero for now.
        break;
        case CHUNK_TYPE_ID_REALLOC:
        writeSize = 0; // Set to zero for now.
        break;
        case CHUNK_TYPE_ID_MEMALIGN:
        writeSize = 0; // Set to zero for now.
        break;
        default:
        writeSize = 0;
        break;
        }
        int written = write(logFileFd, (char*)&dataCache[cacheNumber][i], writeSize);
        (void)written;
    }
    // Unlock writeing
    storeChunkLock.clear();
}


// Store data chunk generated by memory hooks
void DataChunStorage::storeDataChunk(void *dataChunk)
{
    while(true)     // infinite loop until passed chunk is stored in chache
    {
        int currentCache = cacheIndex.load();
        int indexInsideCache = indexStoreing[currentCache].fetch_add(1);
        if(indexInsideCache < CACHE_SIZE)
        {
            dataCache[currentCache][indexInsideCache] = *(DataChunkBase*)dataChunk;
            if(indexStored[currentCache].fetch_add(1) == CACHE_SIZE-1)
            {
                writeCache(currentCache, CACHE_SIZE);
                indexStored[currentCache] = 0;
                indexStoreing[currentCache] = 0;
            }
            return;
        }
        else if(indexInsideCache == CACHE_SIZE)   // Index is now pointing out of the array
        {
            int nextCacheIndex = currentCache+1;
            if(nextCacheIndex >= NUMBER_OF_CACHES)
                nextCacheIndex++;

            while(indexStoreing[nextCacheIndex].load() != 0)
                ;   // Wait until data from next cache are written
            // Increment cache number
            cacheIndex.store(nextCacheIndex);
        }
    }
}


void DataChunStorage::storeSummary(int mallocCount, int freeCount,
                                   int callocCount, int reallocCount,
                                   int memalignCount)
{
    // Open text logging file and truncate it (new summary will be written).
    int summaryLogFileFd = open(outputSummaryFilePath, O_WRONLY | O_CREAT |
                               O_TRUNC,
                               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    // Generate new summary message.
    // This method was chosen because of no memory allocation is presented.
    char string[200];
    sprintf(string, "malloc: %d\nfree: %d\ncalloc: %d\nrealloc: %d\n"
            "memalign: %d\n", mallocCount, freeCount, callocCount, reallocCount,
            memalignCount);
    // Write message into file
    int written = write(summaryLogFileFd, &string, strlen(string));
    (void)written;
    // Close logging file
    close(summaryLogFileFd);
}
