#pragma once

#include <optional>
#include <structure/binary_search.hpp>
#include <structure/record_store.hpp>

template <typename T> std::optional<T> RecordStore::Load(size_t location) {
    return buffer->Read<T>(location);
}

template <typename Key, typename T> RecordStore::CachedBlock* RecordStore::GetBlock(size_t location) {
    if(location == 0) return nullptr;

    struct Record {
        size_t hash;
        Key key;
        T record;
    };

    CachedBlock* block = block_cache.Get(location);
    if (block && sizeof(Record) == block->record_size)
        return block;

    auto header_opt = Load<BlockHeader>(location);
    if (!header_opt)
        return nullptr;

    auto& header = header_opt.value();

    CachedBlock new_block = {header, std::vector<Record>(header.capacity)};

    if (!buffer->Read(location + sizeof(BlockHeader), sizeof(Record) * header.records_total, new_block.data.data()))
        return nullptr;

    auto [evicted_location, evicted] = block_cache.Load(location, new_block);

    if (evicted) {
        buffer->Write(evicted_location, sizeof(BlockHeader), &evicted.header);
        buffer->Write(evicted_location + sizeof(BlockHeader), evicted.data.size(), evicted.data.data());
    }

    return block_cache.Get(location);
}

#define TEMPLATE template <typename Key, typename T, typename Hash, typename Equal>

TEMPLATE
void RecordStore::StoreRecord(int id, Key key, const T& record) {
    struct Record {
        size_t hash;
        Key key;
        T record;
    };
}

TEMPLATE
std::optional<T> RecordStore::FindRecordInternal(size_t block_start, Key key) {
    auto current_block = GetBlock<Key, T>(block_start);

    const static Hash hash_fn;
    const static Equal equal_fn;

    struct Record {
        size_t hash;
        Key key;
        T record;
    };

    while (current_block) {
        size_t hash = hash_fn(key);

        if (current_block->header.type == RecordStore::BlockHeader::WAYSTONE) {
            auto [location_opt, locations_opt] = FindByWaystone(current_block, key);
            if(!location_opt && !locations_opt) return std::nullopt;

            if(location_opt)
                current_block = GetBlock<Key, T>(location_opt.value());
            else{ // Unlikely but branching is possible
                for(auto& location: locations_opt.value()){
                    auto record_option = FindRecordInternal<Key, T, Hash, Equal>(location, key);
                    if(record_option) return record_option;
                }
            }
            continue;
        }

        size_t record_count = current_block->header.records_total;
        Record* record_start = static_cast<Record*>(current_block->data);
        Record* record_end = record_start + record_count;

        Record* found_record = BinarySearch(&Record::hash, hash, record_start, record_count);

        if (!found_record)
            return std::nullopt;

        if (found_record && equal_fn(found_record->key, key))
            return *found_record;

        while (found_record + 1 != record_end && (found_record + 1)->hash == hash) {
            found_record++;
            if (found_record && equal_fn(found_record->key, key))
                return *found_record;
        }

        return std::nullopt;
    }

    return std::nullopt;
}

TEMPLATE
std::optional<T> RecordStore::FindRecord(int id, Key key) {
    if(id < 0 || id > master_records_max) return std::nullopt;
    return FindRecordInternal<Key, T, Hash, Equal>(loaded_header.master_records[id]);
}

#undef TEMPLATE
