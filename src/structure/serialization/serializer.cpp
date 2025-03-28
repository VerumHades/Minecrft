#include <structure/serialization/serializer.hpp>

/*
    General serialization
*/
template <typename T>
bool Serializer::Serialize(const T& value, ByteArray& output) {
    return output.append<T>(value);
}

template <typename T>
bool Serializer::Deserialize(T& value, ByteArray& input) {
    auto opt = input.read<T>(value);
    if(!opt) return false;
    value = opt.value();
    return true;
}

#define SerializeFunction(type) template <typename T> bool Serializer::Serialize<type>(const T& value, ByteArray& output)

#include <blockarray.hpp> 

template <typename T>
bool Serializer::Serialize(const T& value, ByteArray& output) {
    return output.append<T>(value);
}

template <typename T>
bool Serializer::Deserialize(T& value, ByteArray& input) {
    auto opt = input.read<T>(value);
    if(!opt) return false;
    value = opt.value();
    return true;
}

template void myFunction<int>(int);