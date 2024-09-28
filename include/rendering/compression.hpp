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

class BitArray3D{
    uint64 values[64][64];

    public:
        uint64* operator[] (int index)
        {
            return values[index];
        }
        const uint64* operator[](int index) const {
            return values[index];  
        }
        uint64* getAsFlatArray(){
            return reinterpret_cast<uint64*>(values); 
        }
};

typedef uint8_t compressed_byte;

/*
    Structure that can represent a full 64 * 64 * 64 array of bits with a single instance

    00-0.. (22 remaining bits)

    First two bits are for modes: 
        0. ZEROES
        1. ONES
        2. LITERAL 
        4. unused
*/
struct compressed_24bit{
    uint8_t bytes[3];
    
    public:
        // Sets the numerical value, cannot store a whole 32 bit number
        void setValue(uint32_t value);
        uint32_t getValue();
        void setMode(uint8_t mode);
        uint8_t getMode();
};

namespace bitworks{
    std::vector<compressed_24bit> compressBitArray3D(BitArray3D array);

    std::vector<compressed_byte> compress64Bits(uint64 bits);
    uint64 decompress64Bits(std::vector<compressed_byte> bytes);
};

inline uint8_t count_leading_zeros(uint64 x) {
    return std::countl_zero(x);
}

#endif