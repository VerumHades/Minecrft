#pragma once

#pragma once

#include <cstdlib>
#include <optional>

#include <structure/definitions.hpp>

#define WriteBreak(stream, args) if(!stream.Write##args) return false;

/**
 * @brief A generic definition for a stream
 * 
 */
class Stream{
    public:
        /**
         * @brief Read bytes from stream
         * 
         * @param size 
         * @param buffer 
         * @return true 
         * @return false 
         */
        virtual bool Read(size_t size, byte* buffer) = 0;
        /**
         * @brief Write bytes into stream
         * 
         * @param size 
         * @param buffer 
         * @return true 
         * @return false 
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
