#pragma once

#include <map>
#include <unordered_map>

#include <structure/caching/cache.hpp>
#include <structure/serialization/serializer.hpp>
#include <structure/streams/buffer.hpp>
#include <structure/keyed_storage.hpp>

/**
 * @brief A structure that writes into a buffer and stores elements by their respective keys (In large blocks), allows allocation for said blocks
 * 
 * @tparam Key 
 * @tparam OuterHeader an extension to the header
 * @tparam Hash 
 * @tparam Equal 
 */
template <typename Key, typename OuterHeader, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
class RecordStore: public KeyedStorage<Key>{
    public:
    struct Header {
        size_t end = 0;
        size_t free_records_total = 0;

        size_t first_block = 0;
        size_t last_block = 0;

        OuterHeader header{};
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
    const static size_t per_block_record_count = 1024ULL * 1024ULL;
    const static size_t new_allocation_padding = 1024;
    Buffer* buffer = nullptr;

    struct CachedBlock {
        BlockHeader header;
        std::unordered_map<Key, Record, Hash, Equal> records;
    };

    // Free blocks in the format of size, location
    std::multimap<size_t, size_t> free_blocks;
    
    /**
     * @brief Finds space from free blocks (will not give blocks that are > 2 * capacity) or appends to the end to get more space
     * 
     * @param capacity 
     * @return size_t location of newly allocated space
     */
    size_t AllocateBlock(size_t capacity);
    // Does no checks, only registers a block as free
    void FreeBlock(size_t location, size_t capacity);

    Cache<size_t, CachedBlock> block_cache{50};

    void LoadBlockIntoCache(size_t location, const CachedBlock& block);

    CachedBlock* GetRecordBlock(size_t location);
    CachedBlock* CreateNewRecordBlock(size_t capacity);

    void SaveFreeRecords();
    void FlushBlock(size_t location, CachedBlock& block);
    void SaveHeader();

    void Flush();

    Record* Get(const Key& key);

  public:
    RecordStore();
    ~RecordStore();

    /**
     * @brief Save a value at a key
     * 
     * @param key 
     * @param size 
     * @param data 
     */
    void Save(const Key& key, size_t size, const byte* data) override;
    /**
     * @brief Get the value from a key
     * 
     * @param key 
     * @param output 
     * @return true 
     * @return false 
     */
    bool Get(const Key& key, std::vector<byte>& output) override;

    OuterHeader& GetHeader() { return loaded_header.header; };
    const OuterHeader& GetHeader() const { return loaded_header.header; };

    /**
     * @brief Completely reset all stored record, clears data
     * 
     */
    void ResetHeader();

    /**
     * @brief Sets the currently used buffer
     * 
     * @param buffer 
     */
    void SetBuffer(Buffer* buffer);
};

#include <structure/record_store.impl.hpp>
