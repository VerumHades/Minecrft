#pragma once

#include <structure/binary_search.hpp>
#include <structure/record_store.hpp>

#define TEMPLATE template <typename Key, typename T, typename Hash, typename Equal>
#define CLASS RecordStore<Key, T, Hash, Equal>

TEMPLATE
void CLASS::LoadBlockIntoCache(size_t location, const CachedBlock& new_block){
    auto [evicted_location, evicted] = block_cache.Load(location, new_block);

    if (evicted) {
        buffer->Write(evicted_location, sizeof(BlockHeader), &evicted.header);
        buffer->Write(evicted_location + sizeof(BlockHeader), evicted.data.size(), evicted.data.data());
    }
}

TEMPLATE
CLASS::CachedBlock* CLASS::GetBlock(size_t location) {
    if(!buffer) return nullptr;

    if (location == 0)
        return nullptr;

    CachedBlock* block = block_cache.Get(location);
    if (block && sizeof(Record) == block->record_size)
        return block;

    auto header_opt = buffer->Read<BlockHeader>(location);
    if (!header_opt)
        return nullptr;

    auto& header = header_opt.value();

    CachedBlock new_block = {header, {}};
    auto temporary_vector = std::vector<Record>(header.records_total);

    if (!buffer->Read(location + sizeof(BlockHeader), sizeof(Record) * header.records_total, temporary_vector.data()))
        return nullptr;

    for(auto& record: temporary_vector)
        new_block.records[record.key] = record;

    LoadBlockIntoCache(location, new_block);

    return block_cache.Get(location);
}

TEMPLATE
size_t CLASS::CreateNewBlock(size_t capacity) {
    BlockHeader header = {capacity};
    CachedBlock new_block = {header};

    size_t location = loaded_header.end;
    loaded_header.end += sizeof(BlockHeader) + sizeof(Record) * header.capacity;

    LoadBlockIntoCache(location, new_block);

    return block_cache.Get(location);
}

TEMPLATE
void CLASS::SetBuffer(Buffer* buffer) {
    auto header_opt = buffer->Read<Header>(0);

    if (!header_opt) {
        this->buffer = nullptr;
        return;
    }

    loaded_header = header_opt.value();
}

TEMPLATE
void CLASS::ResetHeader() {
    memset(&loaded_header, 0, sizeof(Header));
}

TEMPLATE
void CLASS::SaveHeader() {
    if (!buffer)
        return;
    buffer->Write<Header>(0, loaded_header);
}

#undef TEMPLATE
#undef CLASS
