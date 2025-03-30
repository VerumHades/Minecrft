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
        virtual bool Write(size_t size, byte* buffer) = 0;

        template <typename T>
        bool Write(const T& value){
            return Write(sizeof(T), &value);
        }

        template <typename T>
        std::optional<T> Read(){
            T object{};
            if(!Read(sizeof(T), &object)) return std::nullopt;
            return object;
        }
};
