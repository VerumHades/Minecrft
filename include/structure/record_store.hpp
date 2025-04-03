#pragma once

#include <unordered_map>

#include <structure/allocators/linear_allocator.hpp>
#include <structure/caching/cache.hpp>
#include <structure/serialization/serializer.hpp>
#include <structure/streams/buffer.hpp>

template <typename Key, typename T, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
class RecordStore {
  protected:
    struct Header {
        size_t end;
        size_t first_block;
        size_t last_block;
    } loaded_header;

    struct BlockHeader {
        size_t capacity = 0;      // Total capacity
        size_t records_total = 0; // Actual taken up space
        size_t next_block;
    };

    struct Record{
        Key key;
        size_t location = 0;
        size_t capacity = 0;
        size_t used_size = 0;
    };

  private:
    Buffer* buffer = nullptr;

    struct CachedBlock {
        BlockHeader header;
        std::unordered_map<Key, Record, Hash, Equal> records;
    };

    Cache<size_t, CachedBlock> block_cache;

    void LoadBlockIntoCache(size_t location, const CachedBlock& block);
    CachedBlock* GetBlock(size_t location);
    size_t CreateNewBlock(size_t capacity);


  protected:
    // Zeroes out header
    void ResetHeader();
    // Flushes header to buffer
    void SaveHeader();

  public:
    RecordStore();

    void SetBuffer(Buffer* buffer);
};
