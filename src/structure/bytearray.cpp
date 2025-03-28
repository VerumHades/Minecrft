#include <structure/bytearray.hpp>


bool ByteArray::WriteToStream(Stream& stream){
    if(
        !stream.Write<char>('|') ||
        !stream.Write<size_t>(data.size()) ||
        !stream.Write(data.size(), data.data())
    ) return false;
    return true;
}

bool ByteArray::LoadFromStream(Stream& stream){
    char value = stream.Read<char>();
    if(value != '|') return false;
    size_t size = stream.Read<size_t>();

    data.resize(size);
    return stream.Read(size, reinterpret_cast<byte*>(data.data()));
}

bool ByteArray::operator== (const ByteArray& array){
    if(data.size() != array.data.size()) return false;
    return std::memcmp(data.data(), array.data.data(), data.size()) == 0;
}