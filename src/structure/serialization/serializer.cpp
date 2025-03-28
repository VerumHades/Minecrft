#include <structure/serialization/serializer.hpp>

/*
    General serialization
*/
template <typename T>
bool Serializer::Serialize(T& value, ByteArray& output) {
    return output.append<T>(value);
}

template <typename T>
bool Serializer::Deserialize(T& value, ByteArray& input) {
    auto opt = input.read<T>(value);
    if(!opt) return false;
    value = opt.value();
    return true;
}