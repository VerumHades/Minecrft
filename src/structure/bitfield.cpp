#include <structure/bitfield.hpp>

std::array<uint64_t, 64 * 64>& BitField::data() {
    return _internal_data;
}

const std::array<uint64_t, 64 * 64>& BitField::data() const {
    return _internal_data;
}

bool BitField::set(uint x, uint y, uint z) {
    if (!inBounds(x, y, z))
        return false;

    _internal_data[calculateIndex(x, y)] |= (1ULL << (63 - z));
    return true;
}

bool BitField::reset(uint x, uint y, uint z) {
    if (!inBounds(x, y, z))
        return false;

    _internal_data[calculateIndex(x, y)] &= ~(1ULL << (63 - z));
    return true;
}

bool BitField::get(uint x, uint y, uint z) {
    if (!inBounds(x, y, z))
        return false;

    return _internal_data[calculateIndex(x, y)] & (1ULL << (63 - z));
}

bool BitField::setRow(uint x, uint y, uint64_t value) {
    if (!inBounds(x, y))
        return false;

    _internal_data[calculateIndex(x, y)] = value;
    return true;
}

uint64_t BitField::getRow(uint x, uint y) {
    if (!inBounds(x, y))
        return 0ULL;

    return _internal_data[calculateIndex(x, y)];
}

void BitField::fill(bool value) {

    uint64_t segment = value ? ~0ULL : 0ULL;
    for (int i = 0; i < 64 * 64; i++)
        _internal_data[i] = segment;
}
