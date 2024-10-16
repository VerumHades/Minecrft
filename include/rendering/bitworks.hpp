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

#include <game/blocks.hpp>

typedef uint_fast64_t uint64_f;
typedef std::array<uint64_f, 64> Plane64;
typedef std::array<Plane64, (size_t) BlockTypes::BLOCK_TYPES_TOTAL> PlaneArray64;

uint64_t operator"" _uint64(unsigned long long value);

class BitArray3D{
    uint64_f values[64][64] = {0};

    public:
        static const size_t size_bits = 64 * 64 * 64;
        static const size_t size = 64 * 64;
        
        uint64_f* operator[] (int index)
        {
            return values[index];
        }
        const uint64_f* operator[](int index) const {
            return values[index];  
        }
        uint64_f* getAsFlatArray(){
            return reinterpret_cast<uint64_f*>(values); 
        }

        BitArray3D rotate();

        bool operator == (const BitArray3D& array) const {
            return std::memcmp(values, array.values, size * sizeof(uint64_t)) == 0;
        }
};

class BitPlane{
    std::unique_ptr<std::array<uint_fast8_t,8>>  array8;
    std::unique_ptr<std::array<uint_fast16_t,16>> array16;
    std::unique_ptr<std::array<uint_fast32_t,32>> array32;
    std::unique_ptr<std::array<uint_fast64_t,64>> array64;
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
        
        std::string to_string();
};

namespace bitworks{
    std::vector<compressed_24bit> compressBitArray3D(BitArray3D array);
    BitArray3D decompressBitArray3D(std::vector<compressed_24bit> data);

    std::vector<compressed_byte> compress64Bits(uint64_f bits);
    uint64_f decompress64Bits(std::vector<compressed_byte> bytes);

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

inline uint8_t count_leading_zeros(uint64_f x) {
    return std::countl_zero(x);
}

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
        void read(std::fstream &file);
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