#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <bit>
#include <bitset>
#include <cstdint>
#include <vector>
#include <array>
#include <iostream>

#include <blocks.hpp>

typedef uint_fast64_t uint64;
typedef std::array<uint64, 64> Plane64;
typedef std::array<Plane64, (size_t) BlockTypes::BLOCK_TYPES_TOTAL> PlaneArray64;
typedef std::array<Plane64, 64> BitArray3D;

typedef uint8_t compressed_byte;

namespace bitworks{
    std::vector<compressed_byte> compressBitArray3D(std::array<std::array<uint64,64>,64> array);
    std::vector<compressed_byte> compress64Bits(uint64 bits);
    uint64 decompress64Bits(std::vector<compressed_byte> bytes);
};

inline uint8_t count_leading_zeros(uint64 x) {
    return std::countl_zero(x);
}

#endif