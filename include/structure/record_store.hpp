#pragma once

#include <structure/streams/buffer.hpp>
#include <unordered_map>
#include <unordered_set>

template <typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
class RecordStore {
  private:
    Buffer* buffer = nullptr;

    struct Header {
        size_t first_block; // Start of the first record block
    } loaded_header;

    struct RecordBlockHeader {
        size_t next_block;
        size_t last_block;
        size_t records_total;
    };

    struct Record {
        bool free;

        Key key;
        size_t start;

        size_t size;
        size_t used_size;
    };

    Record* GetRecord(Key key) {
        if (!buffer)
            return nullptr;

        if (loaded_records.contains(key))
            return &loaded_records.at(key);

        RecordBlockHeader* block = GetRecordHeader(loaded_header.first_block);
        while (block != nullptr) {

            block = GetRecordHeader(block->next_block);
        }
    }

    RecordBlockHeader* GetRecordHeader(size_t start) {
        if (!buffer)
            return nullptr;

        auto block_option = buffer->Read<RecordBlockHeader>(start);
        if (!block_option)
            return nullptr;

        loaded_record_blocks[start] = block_option.value();

        return &loaded_record_blocks.at(start);
    }

    std::unordered_set<size_t> certainly_nonexistent_records{};
    std::unordered_map<size_t, Record> loaded_records{};
    std::unordered_map<size_t, RecordBlockHeader> loaded_record_blocks{};

    const size_t block_record_count = 500;

  public:
    void Set(Key key, byte* data, size_t size) {
        if (!buffer)
            return;
    }
    size_t GetSize(Key key) {
        if (!buffer)
            return 0;
    }
    void Get(Key key, byte* data) {
        if (!buffer)
            return;
    }

    void Remove(Key key) {
        if (!buffer)
            return;
    }

    void SetBuffer(Buffer* buffer) {
        auto header_opt = buffer->Read<Header>(0);

        if (!header_opt) {
            this->buffer = nullptr;
            return;
        }

        loaded_header = header_opt.value();
        loaded_records = {};
        certainly_nonexistent_records = {};
    }
};
