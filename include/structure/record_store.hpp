#pragma once

#include <map>
#include <unordered_map>

#include <structure/caching/cache.hpp>
#include <structure/serialization/serializer.hpp>
#include <structure/streams/buffer.hpp>

template <typename Key, typename OuterHeader, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
class RecordStore {
    public:
    struct Header {
        size_t end;
        size_t free_records_total;

        size_t first_block;
        size_t last_block;

        OuterHeader header;
    } loaded_header;

    struct BlockHeader {
        size_t capacity = 0;      // Total capacity
        size_t records_total = 0; // Actual taken up space
        size_t next_block = 0;
    };

    struct Record{
        Key key;
        size_t location = 0;
        size_t capacity = 0;
        size_t used_size = 0;
    };

    struct FreeRecord{
        size_t location = 0;
        size_t capacity = 0;
    };

  private:
    const static size_t per_block_record_count = 5000;
    Buffer* buffer = nullptr;

    struct CachedBlock {
        BlockHeader header;
        std::unordered_map<Key, Record, Hash, Equal> records;
    };

    // Free blocks in the format of size, location
    std::multimap<size_t, size_t> free_blocks;
    /*
        Finds space from free blocks (will not give blocks that are > 2 * capacity) or appends to the end to get more space
        Returns location of newly allocated space
    */
    size_t AllocateBlock(size_t capacity);
    // Does no checks, only registers a block as free
    void FreeBlock(size_t location, size_t capacity);

    Cache<size_t, CachedBlock> block_cache{50};

    void LoadBlockIntoCache(size_t location, const CachedBlock& block);

    CachedBlock* GetRecordBlock(size_t location);
    CachedBlock* CreateNewRecordBlock(size_t capacity);

    void SaveFreeRecords();
    void FlushBlock(size_t location, const CachedBlock& block);
    void SaveHeader();

    void Flush();

  public:
    RecordStore();
    ~RecordStore();

    /*
        Allocates space under the key and returns its start location.
        Makes no guarantess about the contents of the allocated spaces, will not copy any data.

        Optionally can upload data of set size.

        For existing keys uses existing space if its enough otherwise allocates new space.
    */
    size_t Save(const Key& key, size_t capacity, byte* data = nullptr, size_t size = 0);

    /*
        Returns a pointer to a record based on its key if it exists, other operations may invalidate this pointer.
    */
    Record* Get(const Key& key);

    OuterHeader& GetHeader() { return loaded_header.header; };
    const OuterHeader& GetHeader() const { return loaded_header.header; };

    void ResetHeader();
    void SetBuffer(Buffer* buffer);
};

#include <structure/record_store.impl.hpp>
