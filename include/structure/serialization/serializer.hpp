#pragma once

#include <bytearray.hpp>

class Serializer{
    public:
        template <typename T>
        static bool Serialize(T& object, ByteArray& output);

        template <typename T>
        static bool Deserialize(T& object, ByteArray& input);
};