#pragma once


#include <fstream>
#include <cstring>
#include <iomanip>
#include <bit>
#include <bitset>
#include <cstdint>
#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <memory>
#include <variant>
#include <cstring>

#include <path_config.hpp>
/*
    Type that selects the smallest type that fits the set number of bits

    Max size of 64 bits.
*/
template <size_t Bits>
using uint_t = typename std::conditional<
    Bits <= 8, uint8_t,
    typename std::conditional<
        Bits <= 16, uint16_t,
        typename std::conditional<
            Bits <= 32, uint32_t,
            typename std::conditional<
                Bits <= 64, uint64_t,
                void 
            >::type
        >::type
    >::type
>::type;

namespace bitworks{
    template <typename T>
    static inline void saveValue(std::fstream &file, T value){
        file.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    template <typename T>
    static inline T readValue(std::fstream &file) {
        T value;
        file.read(reinterpret_cast<char*>(&value), sizeof(T));
        return value;
    }
};

template <typename T>
inline uint8_t count_leading_zeros(T x) {
    return std::countl_zero(x);
}

template <typename T>
inline uint8_t count_ones(T x) {
    return std::popcount(x);
}

template <int size = 64>
class BitPlane{
    private:
        std::array<uint_t<size>, size> data{};

    public:
        BitPlane(){}
        void set(int x,int y){
            data[y] |= 1 << (63 - x);
        }
        void reset(int x,int y){
            data[y] &= ~(1 << (63 - x));
        }
        bool get(int x, int y){
            return data[y] & (1 << (63 - x));
        }

        uint_t<size>& operator[](std::size_t index) {
            return data[index];
        }

        const uint_t<size>& operator[](std::size_t index) const {
            return data[index];
        }

        void clear(){
            data.fill(0);
        }
};

class BlockBitPlanes{
    private:
        BitPlane<64> mask{};
        std::vector<BitPlane<64>> planes{};
        bool log = false;
    public:
        BlockBitPlanes();

        void setRow(size_t type, size_t row, uint64_t value);
        std::vector<BitPlane<64>>& getPlanes(){ return planes; };

        void clear(){
            mask.clear();
            for(auto& plane: planes) plane.clear();
        }

        BitPlane<64>& getMask() {return mask;}
        void setLog(bool value){log = value;}
};

#include <game/blocks.hpp>