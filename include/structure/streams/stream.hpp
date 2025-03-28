#pragma once

#pragma once

#include <cstdint>


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
        T Read(){
            T object{};
            return Read(sizeof(T), &object);
            return object;
        }
};