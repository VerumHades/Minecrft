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

template <int size>
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
};

using BlockBitPlanes = std::array<BitPlane<64>, 64>;

using byte = uint8_t;

class ByteArray{
    private:
        std::vector<byte> data = {};
        size_t cursor = 0;
    public:    
        ByteArray(){}
        void append(const ByteArray& array){
            data.insert(data.end(), array.data.begin(), array.data.end());
        }
      
        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        void append(const T& data){
            auto* bytePtr = reinterpret_cast<const byte*>(&data);
            this->data.insert(this->data.end(), bytePtr, bytePtr + sizeof(T));
        }
        
        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        void append(const std::vector<T>& source){
            size_t totalSize = source.size() * sizeof(T);
            append<size_t>(source.size()); 

            data.resize(data.size() + totalSize);
            auto* sourceArray = reinterpret_cast<const byte*>(source.data());
            std::memcpy(data.data() + data.size() - totalSize, sourceArray, totalSize);
        }

        void append(const std::string& source){
            size_t totalSize = source.size();
            append<size_t>(source.size()); 

            data.resize(data.size() + totalSize);
            auto* sourceArray = reinterpret_cast<const byte*>(source.data());
            std::memcpy(data.data() + data.size() - totalSize, sourceArray, totalSize);
        }

        std::string sread(){
            size_t size = read<size_t>();

            if(cursor + size > data.size()){
                std::cerr << "Invalid bytearray read: " << size << " at " << cursor << " which is: " << (cursor + size) << " over the total size: " << data.size() << std::endl; 
                return {};
            }
            std::string out(" ",size);
            std::memcpy(out.data(), data.data() + cursor, size);

            cursor += size;
            return out;
        }

        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        T read(){
            T out{};
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

        bool saveToFile(std::string path);

        static ByteArray FromFile(std::string path);
        static ByteArray FromStream(std::fstream &file);

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
    for (size_t i = 0; i < v.getData().size(); ++i) {
        uint8_t byte = v.getData()[i];
        Str << getColorForByte(byte) << std::hex << std::setfill('0') << std::setw(2) << (int)byte;
    }
    Str << "\033[0m";  // Reset the color after printing
    return Str;
}