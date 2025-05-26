#include <bitarray.hpp>
#include <memory>
#include <mutex>

void BitField3D::resetID() {
    id = last_id++;
}

bool BitField3D::set(uint x, uint y, uint z) {
    if (!inBounds(x, y, z))
        return false;

    auto lock = guard.Unique();

    if (transposed_cache_version_pointer) { // Do we have a cached rotated version?
        if (transposed_cache_version_pointer->creator_id == id)
            transposed_cache_version_pointer->field.set(z, y, x); // Is it still ours?
        else
            transposed_cache_version_pointer = nullptr; // Its not
    }

    return BitField::set(x, y, z);
}

bool BitField3D::reset(uint x, uint y, uint z) {
    if (!inBounds(x, y, z))
        return false;

    auto lock = guard.Unique();

    if (transposed_cache_version_pointer) { // Do we have a cached rotated version?
        if (transposed_cache_version_pointer->creator_id == id)
            transposed_cache_version_pointer->field.reset(z, y, x); // Is it still ours?
        else
            transposed_cache_version_pointer = nullptr; // Its not
    }

    return BitField::reset(x, y, z);
}

bool BitField3D::get(uint x, uint y, uint z) {
    if (!inBounds(x, y, z))
        return false;

    auto lock = guard.Shared();
    return BitField::get(x, y, z);
}

bool BitField3D::setRow(uint x, uint y, uint64_t value) {
    if (!inBounds(x, y))
        return false;

    auto lock = guard.Unique();
    return BitField::setRow(x, y, value);
}

uint64_t BitField3D::getRow(uint x, uint y) {
    if (!inBounds(x, y))
        return 0ULL;

    auto lock = guard.Shared();
    return BitField::getRow(x, y);
}

void BitField3D::fill(bool value) {
    auto lock = guard.Unique();
    BitField::fill(value);
}

BitField3D* BitField3D::getTransposed() {
    {
        auto lock = guard.Shared();
        if (transposed_cache_version_pointer && transposed_cache_version_pointer->creator_id == id) {
            return &transposed_cache_version_pointer->field;
        }
    }

    /*
        TODO: implement transposing
    */

    auto transposed = std::make_unique<BitField>();

    {
        auto lock = guard.Shared();
        for (int z = 0; z < 64; z++) {
            for (int y = 0; y < 64; y++) {
                uint64_t value = BitField::getRow(z, y);

                for (int x = 0; x < 64; x++) {
                    uint64_t mask = 1ULL << (63 - x);

                    if (!(value & mask))
                        continue;

                    transposed->setRow(x, y, transposed->getRow(x, y) | (1ULL << (63 - z)));
                }
            }
        }
    }

    auto lock = guard.Unique();
    transposed_cache_version_pointer = BitFieldCache::get().next(id, &transposed->data());

    return &transposed_cache_version_pointer->field;
}

const static std::array<uint64_t, 6> column_masks = {0xAAAAAAAAAAAAAAAA, // 1010
                                                     0x8888888888888888, 0x8080808080808080, 0x8000800080008000,
                                                     0x8000000080000000, 0x8000000000000000};

const static std::array<int, 6> column_offsets = {1, 2, 4, 8, 16, 32};

/*
    Avarages from one level to a level below, will not work properly across levels with gaps
*/
static inline uint64_t fourMemberAvarage(uint64_t row1, uint64_t row2, uint64_t row3, uint64_t row4,
                                         BitField3D::SimplificationLevel level) {
    uint64_t column_mask = column_masks[(size_t)level];
    int offset = column_offsets[(size_t)level];

    /*
        Avarage for each column in all the 4 rows
    */
    // uint64_t whole_avarage = ((row1 | row2) & (row3 | row4)) | ((row1 | row3) & (row2  | row4));

    uint64_t whole_avarage = row1 | row2 | row3 | row4;

    /*
        Avarage for pairs
    */
    uint64_t pair_avarage = (whole_avarage & column_mask) | ((whole_avarage << offset) & column_mask);

    return pair_avarage;
}

static inline uint64_t expand(uint64_t value, BitField3D::SimplificationLevel level) {
    // Expands the ones to match the levels base size
    for (int i = 1; i != (2 << level); i <<= 1)
        value |= (value >> i);

    return value;
}

BitField3D* BitField3D::getSimplified(SimplificationLevel level) {
    if (simplified_version_pointer && simplified_version_pointer->creator_id == id) {
        return &simplified_version_pointer->field;
    }
    if (level == NONE)
        throw std::logic_error("Cannot simplify mesh to NONE level.");

    auto output = BitFieldCache::get().next(id);
    simplified_version_pointer = output;

    auto& field = output->field;

    for (int step = 0; step <= level; step++) {
        int offset = column_offsets[step];
        int group_offset = offset * 2;

        auto& source_field = step == 0 ? *this : output->field;

        for (int x = 0; x < 64; x += group_offset)
            for (int y = 0; y < 64; y += group_offset) {
                field.setRow(x, y,
                             fourMemberAvarage(source_field.getRow(x, y), source_field.getRow(x + offset, y),
                                               source_field.getRow(x, y + offset),
                                               source_field.getRow(x + offset, y + offset),
                                               static_cast<SimplificationLevel>(step)));
            }
    }

    int group_offset = column_offsets[level] * 2;
    for (int x = 0; x < 64; x += group_offset)
        for (int y = 0; y < 64; y += group_offset) {
            uint64_t core = expand(field.getRow(x, y), level);

            for (int i = 0; i < group_offset; i++)
                for (int j = 0; j < group_offset; j++) {
                    field.setRow(x + i, y + j, core);
                }
        }

    return &output->field;
}

BitField3D* BitField3D::getSimplifiedWithNone(SimplificationLevel level) {
    if (level == NONE)
        return this;
    else
        return getSimplified(level);
}

CompressedArray BitField3D::getCompressed() {
    return compress(data());
}

const uint64_t all_zeroes = 0ULL;
const uint64_t all_ones = ~0ULL;

void compressPlane(const std::array<uint64_t, 64>& plane, CompressedArray& output) {
    CompressedArray compressed_rows{};
    compressed_rows.reserve(64);

    uint64_t row_compression_mask = 0;
    uint64_t row_value_mask = 0;

    for (int i = 0; i < 64; i++) {
        auto& row = plane[i];

        if (row == all_zeroes || row == all_ones) {
            row_compression_mask |= (1ULL << i);
            if (row == all_ones)
                row_value_mask |= (1ULL << i);
        } else
            compressed_rows.push_back(row);
    }

    output.push_back(row_compression_mask);
    output.push_back(row_value_mask);
    output.insert(output.end(), compressed_rows.begin(), compressed_rows.end());
}

std::tuple<std::array<uint64_t, 64>, uint64_t*> decompressPlane(uint64_t* input) {
    std::array<uint64_t, 64> output;

    uint64_t row_compression_mask = *input++;
    uint64_t row_value_mask = *input++;
    uint64_t* uncompressed_rows = input;

    for (int i = 0; i < 64; i++) {
        auto& row = output[i];

        if (row_compression_mask & (1ULL << i))
            row = (row_value_mask & (1ULL << i)) ? all_ones : all_zeroes;

        else
            row = *uncompressed_rows++;
    }

    return {output, uncompressed_rows};
}

/*
    Row based compression
*/
CompressedArray BitField3D::compress(const std::array<uint64_t, 64 * 64>& source) {
    CompressedArray data_output{};
    data_output.reserve(64 * 64);

    std::array<uint64_t, 64> compression_mask{};
    std::array<uint64_t, 64> value_mask{};

    for (int x = 0; x < 64; x++)
        for (int y = 0; y < 64; y++) {
            auto& row = source[calculateIndex(x, y)];

            if (row == all_zeroes || row == all_ones) {
                compression_mask[y] |= (1ULL << x);
                if (row == all_ones)
                    value_mask[y] |= (1ULL << x);
            } else
                data_output.push_back(row);
        }

    CompressedArray output{};
    output.reserve(data_output.size() + 64 * 2);

    compressPlane(compression_mask, output);
    compressPlane(value_mask, output);

    output.insert(output.end(), data_output.begin(), data_output.end());

    output.shrink_to_fit();
    return output;
}

void BitField3D::decompress(std::array<uint64_t, 64 * 64>& destination, CompressedArray& source) {
    if (source.size() == 0)
        return;

    auto [compression_mask, post_compression_mask] = decompressPlane(source.data());
    auto [value_mask, post_value_mask] = decompressPlane(post_compression_mask);
    uint64_t* data = post_value_mask;

    for (int x = 0; x < 64; x++)
        for (int y = 0; y < 64; y++) {
            auto& row = destination[calculateIndex(x, y)];

            if (compression_mask[y] & (1ULL << x))
                row = (value_mask[y] & (1ULL << x)) ? all_ones : all_zeroes;

            else
                row = *data++;
        }
}

CCacheMember* CompressedBitFieldCache::next(std::shared_ptr<CompressedArray> new_compressed_data) {
    std::lock_guard<std::mutex> lock(mutex);

    next_spot = (next_spot + 1) % (max_cached - 1);

    auto& member = cached_fields[next_spot];
    if (member.compressed_data)
        *member.compressed_data = BitField3D::compress(member.field.data());

    member.compressed_data = new_compressed_data;
    member.field.resetID();
    member.field.fill(0); // Decompression expects a zeroed out field

    BitField3D::decompress(member.field.data(), *new_compressed_data);

    return &member;
}

BitField3D* CompressedBitField3D::get() {
    if (cached_ptr && cached_ptr->compressed_data.get() == data_ptr.get())
        return &cached_ptr->field;

    cached_ptr = CompressedBitFieldCache::get().next(data_ptr);
    return &cached_ptr->field;
}
const CompressedArray& CompressedBitField3D::getCompressed() {
    if (cached_ptr && cached_ptr->compressed_data.get() == data_ptr.get())
        *data_ptr = BitField3D::compress(cached_ptr->field.data());

    return *data_ptr;
}

void CompressedBitField3D::set(const BitField3D& source) {
    cached_ptr = nullptr;
    data_ptr = std::make_shared<CompressedArray>(BitField3D::compress(source.data()));
}

CompressedBitField3D::CompressedBitField3D() {
    data_ptr = std::make_shared<CompressedArray>();
}
