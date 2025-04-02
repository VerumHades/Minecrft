
#include "structure/binary_search.hpp"
#include <cstring>
#include <optional>
#include <structure/record_store.hpp>

void RecordStore::SetBuffer(Buffer* buffer) {
    auto header_opt = buffer->Read<Header>(0);

    if (!header_opt) {
        this->buffer = nullptr;
        return;
    }

    loaded_header = header_opt.value();
}

std::tuple<std::optional<size_t>, std::optional<std::vector<size_t>>>
RecordStore::FindByWaystone(CachedBlock* waystone_block, size_t hash) {
    if (waystone_block->header.type != BlockHeader::WAYSTONE)
        return {std::nullopt, std::nullopt};

    size_t record_count = waystone_block->header.records_total;
    WaystoneRecord* records = static_cast<WaystoneRecord*>(waystone_block->data);

    int index = BinarySearchOrder(&WaystoneRecord::lowest_hash, hash, records, record_count);
    if (index < 0 || index >= record_count)
        return {std::nullopt, std::nullopt}; // Out of bounds

    if (index + 1 < record_count && records[index].largest_hash == records[index + 1].lowest_hash) { // Special case
        std::vector<size_t> possibilities = {records[index].block_start};
        while (index + 1 < record_count && records[index].largest_hash == records[index + 1].lowest_hash)
            possibilities.push_back(records[index + 1].block_start);

        return {std::nullopt, possibilities};
    } else {
        return {records[index].block_start, std::nullopt};
    }
    return {std::nullopt, std::nullopt};
}

void RecordStore::ResetHeader() {
    memset(&loaded_header, 0, sizeof(Header));
}

void RecordStore::SaveHeader() {
    if (!buffer)
        return;
    buffer->Write<Header>(0, loaded_header);
}
