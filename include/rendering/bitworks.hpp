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

struct CompressedArray{
    std::vector<compressed_24bit> data;
    size_t source_size;
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

class DynamicBitArray3D;
/*
    A simple dynamic 2D array saved in memory as a contiguous block
*/
class Dynamic2DArray{
    virtual size_t getSize() = 0;
    virtual uint_t<64> get(uint32_t x, uint32_t y) = 0;
    virtual void set(uint32_t x, uint32_t y, uint_t<64> value);
};

template <typename T>
class Array2D: public Dynamic2DArray{
    private:
        std::vector<T> data;

        friend class DynamicBitArray3D;
    
    public:
        uint_t<64> get(uint32_t x, uint32_t y){
            if(x < 0 || x >= size || y < 0 || y >= size) return 0ULL;
            
            switch(actualSize){
                case BIT8 : return internalArray8Bit .get(x,y);
                case BIT16: return internalArray16Bit.get(x,y);
                case BIT32: return internalArray32Bit.get(x,y);
                case BIT64: return internalArray64Bit.get(x,y);
            }
        }
        void set(uint32_t x, uint32_t y, uint_t<64> value){
            if(x < 0 || x >= size || y < 0 || y >= size) return;

            switch(actualSize){
                case BIT8 : internalArray8Bit .get(x,y) = value; break;
                case BIT16: internalArray16Bit.get(x,y) = value; break;
                case BIT32: internalArray32Bit.get(x,y) = value; break;
                case BIT64: internalArray64Bit.get(x,y) = value; break;
            }
        }

        T& get(size_t x, size_t y){ 
            return data[x + (y * size)];   
        }
        ~Dynamic2DArray(){
            if(data) delete[] data;
        }
        
        T* getData() { return data; };

        size_t getSize() override {return size;}
};

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
    uint_t<bits>* decompress(CompressedArray compressed_data);
};

template <typename T>
inline uint8_t count_leading_zeros(T x) {
    return std::countl_zero(x);
}

#include <rendering/bitworks.tpp>
/*
    A 3D array of bits with a dynamic size.
    (A 2D array of unsigned integers)

    Max size of 64 (Size is size in all directions, total amount of bits would be 64^3)
*/
class DynamicBitArray3D{
    private:        
        Dynamic2DArray<uint_t<8> > internalArray8Bit ;
        Dynamic2DArray<uint_t<16>> internalArray16Bit;
        Dynamic2DArray<uint_t<32>> internalArray32Bit;
        Dynamic2DArray<uint_t<64>> internalArray64Bit;

        size_t size = 0;
        enum{
            BIT8,
            BIT16,
            BIT32,
            BIT64
        } actualSize;

    public:
        DynamicBitArray3D(size_t size): size(size){
            if     (size <= 8)  { internalArray8Bit .setup(size); actualSize = BIT8 ; }
            else if(size <= 16) { internalArray16Bit.setup(size); actualSize = BIT16; }
            else if(size <= 32) { internalArray32Bit.setup(size); actualSize = BIT32; }
            else if(size <= 64) { internalArray64Bit.setup(size); actualSize = BIT64; }
            else std::runtime_error("DynamicBitArray3D has a max size of 64!");
        }

        DynamicBitArray3D(size_t size, CompressedArray data): size(size){
            if     (size <= 8)  { internalArray8Bit .setup(size, bitworks::decompress<8> (data)); actualSize = BIT8 ; }
            else if(size <= 16) { internalArray16Bit.setup(size, bitworks::decompress<16>(data)); actualSize = BIT16; }
            else if(size <= 32) { internalArray32Bit.setup(size, bitworks::decompress<32>(data)); actualSize = BIT32; }
            else if(size <= 64) { internalArray64Bit.setup(size, bitworks::decompress<64>(data)); actualSize = BIT64; }
            else std::runtime_error("DynamicBitArray3D has a max size of 64!");
        }
        
        uint_t<64> get(uint32_t x, uint32_t y){
            if(x < 0 || x >= size || y < 0 || y >= size) return 0ULL;
            
            switch(actualSize){
                case BIT8 : return internalArray8Bit .get(x,y);
                case BIT16: return internalArray16Bit.get(x,y);
                case BIT32: return internalArray32Bit.get(x,y);
                case BIT64: return internalArray64Bit.get(x,y);
            }
        }
        void set(uint32_t x, uint32_t y, uint_t<64> value){
            if(x < 0 || x >= size || y < 0 || y >= size) return;

            switch(actualSize){
                case BIT8 : internalArray8Bit .get(x,y) = value; break;
                case BIT16: internalArray16Bit.get(x,y) = value; break;
                case BIT32: internalArray32Bit.get(x,y) = value; break;
                case BIT64: internalArray64Bit.get(x,y) = value; break;
            }
        }

        void set(uint32_t x, uint32_t y, uint32_t z){
            if(z < 0 || z >= size) return;

            set(x,y, get(x,y) | (1ULL << (size - 1 - z)));
        }

        void reset(uint32_t x, uint32_t y, uint32_t z){
            if(z < 0 || z >= size) return;

            set(x,y, get(x,y) & ~(1ULL << (size - 1 - z)));
        }

        bool get(uint32_t x, uint32_t y, uint32_t z){
            if(z < 0 || z >= size) return false;

            return get(x,y) & (1ULL << (size - 1 - z)); 
        }

        CompressedArray compress(){
            switch(actualSize){
                case BIT8 : return bitworks::compress<8 >(internalArray8Bit.getData() , size * size);
                case BIT16: return bitworks::compress<16>(internalArray16Bit.getData(), size * size); 
                case BIT32: return bitworks::compress<32>(internalArray32Bit.getData(), size * size); 
                case BIT64: return bitworks::compress<64>(internalArray64Bit.getData(), size * size); 
            }
        }

        DynamicBitArray3D getRotated();
        size_t getSize(){return size;}
};

template <int size>
using BitPlane = std::array<uint_t<size>, size>;

template <int size>
using BlockBitPlanes = std::array<BitPlane<size>, (size_t) BlockTypes::BLOCK_TYPES_TOTAL>;

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