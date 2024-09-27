#include <rendering/compression.hpp>


inline uint8_t getMode(compressed_byte member){
    return (0b11000000 & member) >> 6;
}
inline uint8_t getCount(compressed_byte member){
    return 0b00111111 & member;
}
inline void setMode(compressed_byte& member, uint8_t mode){
    member |= (mode << 6);
}

uint64_t firstBitMask = (1ULL << 63);
/*
    Compresses a 64 bit uint into an array of 8 bits, will almost never go over the original size only in the worst case scenario of alternating bits.
    
    00-000000
    
    First two bits are for modes: 
        0. ZEROES
        1. ONES
        2. LITERAL 
        4. unused

    The remaining bits are a count max at 64  
*/
std::vector<compressed_byte> bitworks::compress64Bits(uint64 bits){
    std::vector<compressed_byte> out;
    out.reserve(10);
    uint8_t compressed_total = 0;

    while(compressed_total < 64){
        uint8_t start_value = (firstBitMask & bits) >> 63; // Check what the current first value is
        uint8_t count = count_leading_zeros(!start_value ? bits : ~bits);
        
        if(compressed_total + count > 64) count = 64 - compressed_total; // Dont go over 64 bit boundary

        if(count <= 8){ // Compressing 8 bits or less into counts is not good, use a literal instead
            compressed_byte current = bits >> (64 - 6); // shift so that the first six bits are in the right place
            setMode(current, 2); // set the mode to literal

            //std::cout << std::bitset<64>(bits) << std::endl;
            //std::cout << std::bitset<8>(current) << std::endl;

            bits <<= 6; // Shift by the 6 literal bits
            compressed_total += 6;
            out.push_back(current);

            continue;
        }

        compressed_byte current = count;
        setMode(current, start_value); // set the mode
        
        bits <<= count;
        compressed_total += count;
        out.push_back(current);
        

        //std::cout << static_cast<int>(count) << std::endl;
    }

    return out;
}
uint64 bitworks::decompress64Bits(std::vector<compressed_byte> bytes){
    uint64 out = 0;
    uint64 mask = ~0ULL;

    uint8_t currentShift = 0;
    for(auto& value: bytes){
        switch (getMode(value))
        {
        case 0: // set zeroes
            out &= ~mask;
            mask >>= getCount(value); 
            currentShift += getCount(value); 
            break;
        case 1: // Set ones
            out |= mask;
            mask >>= getCount(value); 
            currentShift += getCount(value); 
        case 2: // Set literal
            out &= ~mask;
            out |= (static_cast<uint64_t>(getCount(value)) << ((64 - 6) - currentShift));
            mask >>= 6;
            currentShift += 6;
        default:
            break;
        }
    }

    return out;
}

std::vector<compressed_byte> bitworks::compressBitArray3D(std::array<std::array<uint64,64>,64> array){
    return {};
}