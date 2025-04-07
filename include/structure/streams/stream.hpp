#pragma once

#pragma once

#include <cstdlib>
#include <cstdint>
#include <optional>

#define WriteBreak(stream, args) if(!stream.Write##args) return false;

using byte = uint8_t;
class Stream{
    public:
        /*
            Read bytes from stream
        */
        virtual bool Read(size_t size, byte* buffer) = 0;
        /*
            Write bytes into stream
        */
        virtual bool Write(size_t size, const byte* buffer) = 0;

        template <typename T>
        bool Write(const T& value){
            return Write(sizeof(T), reinterpret_cast<const byte*>(&value));
        }

        template <typename T>
        std::optional<T> Read(){
            T object{};
            if(!Read(sizeof(T), reinterpret_cast<byte*>(&object))) return std::nullopt;
            return object;
        }
};
