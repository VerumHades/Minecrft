#pragma once

#include <structure/bytearray.hpp>

/**
 * @brief The core serialized template, implementations for different elements are in source files
 * 
 */
class Serializer{
    public:
        template <typename T>
        static bool Serialize(T& object, ByteArray& output);
        template <typename T>
        static bool Deserialize(T& object, ByteArray& input);
};
