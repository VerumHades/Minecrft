#pragma once

#include <basetsd.h>
#include <cstdint>
#include <vector>

#include <structure/allocators/linear_allocator.hpp>
#include <structure/caching/cache.hpp>
#include <structure/serialization/serializer.hpp>
#include <structure/streams/buffer.hpp>

class RecordStore {
  protected:
    constexpr static size_t master_records_max = 0;
    struct Header {
        size_t end;
        size_t master_records[master_records_max];
    } loaded_header;

    struct BlockHeader {
        enum Type { RECORD, WAYSTONE } type;
        size_t capacity = 0;      // Total capacity
        size_t records_total = 0; // Actual taken up space
    };

    struct WaystoneRecord {
        size_t block_start;

        size_t lowest_hash;
        size_t largest_hash;
    };

  private:
    Buffer* buffer = nullptr;

    struct CachedBlock {
        BlockHeader header;

        size_t record_size;
        std::vector<uint8_t> data;
    };

    Cache<size_t, CachedBlock> block_cache;

    template <typename T> std::optional<T> Load(size_t location);
    template <typename Key, typename T> CachedBlock* GetBlock(size_t location);
    template <typename Key, typename T> CachedBlock* GetMasterRecord(int id);

    /*
        Finds where to go when looking for a hash in the waystone tree,

        Tho very unlikely a hash could possibly span multiple waystone blocks in some limited scenarios so this function returns all the possible ones to check,
        will return just one index where applicable, and in rare cases the second part will have a value.

        In every other case everything is nullopt.

        one index => index, std::nullopt
        more indices => std::nullopt, {index array}
        not found => std::nullopt, std::nullopt
    */
    std::tuple<std::optional<size_t>, std::optional<std::vector<size_t>>> FindByWaystone(CachedBlock* waystone_block, size_t hash);

    template <typename Key, typename T, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
    std::optional<T> FindRecordInternal(size_t block_start, Key key);

  protected:
    template <typename Key, typename T, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
    void StoreRecord(int id, Key key, const T& record);

    template <typename Key, typename T, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
    std::optional<T> FindRecord(int id, Key key);

    // Zeroes out header
    void ResetHeader();
    // Flushes header to buffer
    void SaveHeader();

  public:
    RecordStore();

    void SetBuffer(Buffer* buffer);
};
