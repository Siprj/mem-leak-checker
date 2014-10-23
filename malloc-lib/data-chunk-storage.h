#ifndef DATACHUNKSTORAGE_H
#define DATACHUNKSTORAGE_H

extern "C"
{

namespace DataChunStorage
{
    void initDataChunkStorage();
    void deinitDataChunkStorage();
    void storeDataChunk(void* dataChunk);

    void storeSummary(int allocationCount, int deallocationCount,
                             int callocCount, int reallocCount,
                             int memalignCount);
}

}

#endif // DATACHUNKSTORAGE_H
