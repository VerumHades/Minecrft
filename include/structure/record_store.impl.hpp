#pragma once

#include "logging.hpp"
#include <structure/binary_search.hpp>
#include <structure/record_store.hpp>

#define TEMPLATE template <typename Key, typename OuterHeader, typename Hash, typename Equal>
#define CLASS RecordStore<Key, OuterHeader, Hash, Equal>

TEMPLATE
CLASS::~RecordStore() {
    Flush();
}

TEMPLATE
CLASS::RecordStore() {}

TEMPLATE
CLASS::Record* CLASS::Get(const Key& key) {
    if (loaded_header.first_block == 0)
        return nullptr;


    CachedBlock* block = GetRecordBlock(loaded_header.first_block);
    while (block != nullptr) {
        if (block->records.contains(key))
            return &block->records.at(key);

        block = GetRecordBlock(block->header.next_block);
    }

    return nullptr;
}

TEMPLATE
bool CLASS::Get(const Key& key, std::vector<byte>& output) {
    if (!buffer)
        return false;


    auto* record = Get(key);

    if (!record)
        return false;

    output.resize(record->used_size);
    buffer->Read(record->location, record->used_size, output.data());

    return true;
}

TEMPLATE
void CLASS::Save(const Key& key, size_t size, const byte* data) {
    if (!buffer)
        return;

    auto existing_option = Get(key);
    size_t new_size = size + new_allocation_padding;

    if (existing_option) {
        if (existing_option->capacity < size) {
            FreeBlock(existing_option->location, existing_option->capacity);

            existing_option->location = AllocateBlock(new_size);
            existing_option->capacity = new_size;
        }

        if (data) {
            buffer->Write(existing_option->location, size, data);
            existing_option->used_size = size;
        }

        return;
    }

    CachedBlock* block = nullptr;

    if (loaded_header.first_block == 0)
        block = CreateNewRecordBlock(per_block_record_count);
    else
        block = GetRecordBlock(loaded_header.last_block);

    if (block->records.size() >= block->header.capacity)
        block = CreateNewRecordBlock(per_block_record_count);

    size_t location = AllocateBlock(new_size);

    block->records[key] = Record{key, location, new_size, 0};

    if (data) {
        buffer->Write(location, size, data);
        block->records.at(key).used_size = size;
    }
}

TEMPLATE
void CLASS::LoadBlockIntoCache(size_t location, const CachedBlock& new_block) {
    auto evicted = block_cache.Load(location, new_block);

    if (evicted)
        FlushBlock(evicted->first, evicted->second);
}

TEMPLATE
void CLASS::FlushBlock(size_t location, CachedBlock& block) {
    auto temporary_vector = std::vector<Record>();

    block.header.records_total = block.records.size();
    temporary_vector.reserve(block.header.records_total);
    for (auto& [key, record] : block.records)
        temporary_vector.push_back(record);

    buffer->Write<BlockHeader>(location, block.header);
    buffer->Write(location + sizeof(BlockHeader), temporary_vector.size() * sizeof(Record),
                  reinterpret_cast<byte*>(temporary_vector.data()));
}

TEMPLATE
CLASS::CachedBlock* CLASS::GetRecordBlock(size_t location) {
    if (!buffer)
        return nullptr;

    if (location == 0)
        return nullptr;

    CachedBlock* block = block_cache.Get(location);
    if (block)
        return block;

    auto header_opt = buffer->Read<BlockHeader>(location);
    if (!header_opt)
        return nullptr;

    auto& header = header_opt.value();

    CachedBlock new_block = {header, {}};
    auto temporary_vector = std::vector<Record>(header.records_total);

    if (!buffer->Read(location + sizeof(BlockHeader), sizeof(Record) * header.records_total,
                      reinterpret_cast<byte*>(temporary_vector.data())))
        return nullptr;

    for (int i = 0;i < header.records_total;i++){
        new_block.records[temporary_vector[i].key] = temporary_vector[i];
    }

    LoadBlockIntoCache(location, new_block);

    return block_cache.Get(location);
}

TEMPLATE
void CLASS::FreeBlock(size_t location, size_t capacity) {
    free_blocks.insert({location, capacity});
}

TEMPLATE
size_t CLASS::AllocateBlock(size_t capacity) {
    auto it = free_blocks.lower_bound(capacity);

    if (it != free_blocks.end()) {
        auto [key, value] = *it;
        if (key < capacity * 2) {
            free_blocks.erase(it);
            return value;
        }
    }

    size_t location = loaded_header.end;
    loaded_header.end += capacity;

    return location;
}

TEMPLATE
CLASS::CachedBlock* CLASS::CreateNewRecordBlock(size_t capacity) {
    BlockHeader header = {capacity};
    CachedBlock new_block = {header};

    size_t location = AllocateBlock(sizeof(BlockHeader) + sizeof(Record) * header.capacity);

    auto last_block = GetRecordBlock(loaded_header.last_block);
    if (last_block)
        last_block->header.next_block = location;
    else
        loaded_header.first_block = location;

    loaded_header.last_block = location;
    LoadBlockIntoCache(location, new_block);

    return block_cache.Get(location);
}

TEMPLATE
void CLASS::Flush() {
    block_cache.Clear([this](auto key, auto block) { FlushBlock(key, block); });
    SaveFreeRecords();
    free_blocks.clear();
    SaveHeader();
}

TEMPLATE
void CLASS::SetBuffer(Buffer* buffer) {
    if (this->buffer != nullptr)
        Flush();

    if (buffer == nullptr) {
        this->buffer = nullptr;
        return;
    }

    this->buffer = buffer;

    auto header_opt = buffer->Read<Header>(0);

    if (!header_opt)
        ResetHeader();

    header_opt = buffer->Read<Header>(0);

    if (!header_opt) {
        this->buffer = nullptr;
        LogError("Invalid buffer for record store.");
        return;
    }

    loaded_header = header_opt.value();

    if (loaded_header.free_records_total == 0)
        return;

    auto free_records = std::vector<FreeRecord>(loaded_header.free_records_total);
    buffer->Read(loaded_header.end, sizeof(FreeRecord) * loaded_header.free_records_total,
                 reinterpret_cast<byte*>(free_records.data()));

    for (auto& [location, capacity] : free_records)
        FreeRecord(location, capacity);
}

TEMPLATE
void CLASS::ResetHeader() {
    loaded_header = {};
    loaded_header.end = sizeof(Header);
    SaveHeader();
}

TEMPLATE
void CLASS::SaveFreeRecords() {
    loaded_header.free_records_total = free_blocks.size();

    if (loaded_header.free_records_total == 0)
        return;

    auto free_records = std::vector<FreeRecord>();
    free_records.reserve(loaded_header.free_records_total);

    for (auto& [location, capacity] : free_blocks)
        free_records.push_back({location, capacity});

    buffer->Write(loaded_header.end, sizeof(FreeRecord) * loaded_header.free_records_total,
                  reinterpret_cast<byte*>(free_records.data()));
}

TEMPLATE
void CLASS::SaveHeader() {
    if (!buffer)
        return;

    buffer->Write<Header>(0, loaded_header);
}

#undef TEMPLATE
#undef CLASS
