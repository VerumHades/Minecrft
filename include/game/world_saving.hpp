#pragma once

#include <game/world.hpp>

using byte = uint8_t;

class ByteArray{
    private:
        std::vector<byte> data;
        size_t cursor;
    public:      
        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        void append(T data);

        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        void append(std::vector<T> data);
        
        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        T read();

        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        std::vector<T> vread();


        void write(std::ofstream &file);
        void read(std::ifstream &file);
};