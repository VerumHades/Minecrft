#include <structure/serialization/serializer.hpp>

/*
    General serialization
*/
template <typename T>
bool Serializer::Serialize(T& value, ByteArray& output) {
    return output.Append<T>(value);
}

template <typename T>
bool Serializer::Deserialize(T& value, ByteArray& input) {
    auto opt = input.Read<T>(value);
    if(!opt) return false;
    value = opt.value();
    return true;
}
