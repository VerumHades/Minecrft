#pragma once

#include <bytearray.hpp>

class Serializer{
    public:
        template <typename T>
        bool Serialize(T& object, ByteArray& output);

        template <typename T>
        bool Deserialize(T& object, ByteArray& input);
};