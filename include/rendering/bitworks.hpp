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

#include <game/blocks.hpp>

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
        
        std::string to_string();
};
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

    struct CompressedArray{
        std::vector<compressed_24bit> data;
        size_t source_size;
    };

    /*
        Compresses an array of unsigned integers to an array of 3 byte chunks that each represents a number of consecutive ones or zeroes

        - array => the array to be compressed
        - size  => the length of the array
    */
    template <int bits>
    CompressedArray compress(uint_t<bits>* array, size_t size);

    /*
        Converts a series of 3 byte values that indicate a number of ones or zeros to an actual array of ones and zeroes

        - compressed_data => data for decompression
    */
    template <int bits>
    void decompress(CompressedArray compressed_data, std::vector<uint_t<bits>>& result);
};

template <typename T>
inline uint8_t count_leading_zeros(T x) {
    return std::countl_zero(x);
}

#include <rendering/bitworks.tpp>

template <int size>
class BitPlane{
    private:
        std::array<uint_t<size>, size> data = {0};

    public:
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
};

template <size_t blocks_total>
using BlockBitPlanes = std::array<BitPlane<64>, blocks_total>;

using byte = uint8_t;

class ByteArray{
    private:
        std::vector<byte> data = {};
        size_t cursor = 0;
    public:      
        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        void append(T data){
            byte* bytePtr = reinterpret_cast<byte*>(&data);
            this->data.insert(this->data.end(), bytePtr, bytePtr + sizeof(T));
        }

        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        void append(std::vector<T> source){
            size_t totalSize = source.size() * sizeof(T);
            append<size_t>(source.size()); 

            data.resize(data.size() + totalSize);
            byte* sourceArray = reinterpret_cast<byte*>(source.data());
            std::memcpy(data.data() + data.size() - totalSize, sourceArray, totalSize);
        }
        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        T read(){
            T out;
            if(cursor + sizeof(T) > data.size()){
                std::cerr << "Invalid bytearray read: " << sizeof(T) << std::endl; 
                return out;
            }

            std::memcpy(&out, data.data() + cursor, sizeof(T));
            cursor += sizeof(T);
            return out;
        }
        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        std::vector<T> vread(){
            size_t size = read<size_t>();
            size_t arraySize = size * sizeof(T);

            if(cursor + arraySize > data.size()){
                std::cerr << "Invalid bytearray read: " << arraySize << " at " << cursor << " which is: " << (cursor + arraySize) << " over the total size: " << data.size() << std::endl; 
                return {};
            }
            std::vector<T> out;
            out.resize(size);

            std::memcpy(out.data(), data.data() + cursor, arraySize);

            cursor += arraySize;

            return out;
        }

        bool operator == (const ByteArray& array);
        void write(std::fstream &file);
        bool read(std::fstream &file);
        size_t getFullSize() {return sizeof(char) + sizeof(size_t) + data.size() * sizeof(byte);};

        const std::vector<byte>& getData() const {return data;}
};
// Helper function to get color based on byte value
inline std::string getColorForByte(uint8_t byte) {
    if (byte < 0x20) return "\033[31m";  // Red for values 0x00 - 0x1F
    if (byte < 0x40) return "\033[32m";  // Green for values 0x20 - 0x3F
    if (byte < 0x60) return "\033[33m";  // Yellow for values 0x40 - 0x5F
    if (byte < 0x80) return "\033[34m";  // Blue for values 0x60 - 0x7F
    if (byte < 0xA0) return "\033[35m";  // Magenta for values 0x80 - 0x9F
    if (byte < 0xC0) return "\033[36m";  // Cyan for values 0xA0 - 0xBF
    return "\033[37m";                   // White for values 0xC0 and above
}

// Overload operator<< for ByteArray with colored hex output
inline std::ostream& operator<<(std::ostream & Str, const ByteArray& v) {
    for (int i = 0; i < v.getData().size(); ++i) {
        uint8_t byte = v.getData()[i];
        Str << getColorForByte(byte) << std::hex << std::setfill('0') << std::setw(2) << (int)byte;
    }
    Str << "\033[0m";  // Reset the color after printing
    return Str;
}