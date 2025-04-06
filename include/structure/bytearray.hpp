#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <structure/streams/stream.hpp>
#include <structure/streams/streamable.hpp>

#include <logging.hpp>
#include <path_config.hpp>

class ByteArray : Streamable {
  private:
    std::vector<byte> data = {};

    size_t cursor = 0;

  public:
    ByteArray() {}

    /*
    Write at offset (in bytes) elements of T, of set count
    Returns total size written in bytes
    */
    template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
    size_t Write(size_t offset, size_t count, const T* source) {
        while(offset + sizeof(T) * count > data.size())
            data.resize(data.size() * 2);

        std::memcpy(data.data() + offset, source, sizeof(T) * count);

        return sizeof(T) * count;
    }

    size_t Write(size_t offset, const ByteArray& array) {
        return Write<byte>(offset, array.data.size(), array.data.data());
    }

    template <typename T> size_t Write(size_t offset, const T& value) {
        return Write<T>(offset, 1, &value);
    }

    template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
    size_t Write(size_t offset, const std::vector<T>& source) {
        size_t size_written = 0;

        size_written += Write<size_t>(offset, source.size());
        size_written += Write<T>(offset + sizeof(size_t), source.size(), source.data());

        return size_written;
    }

    size_t Write(size_t offset, const std::string& source) {
        size_t size_written = 0;

        size_written += Write<size_t>(offset, source.size());
        size_written += Write(offset + sizeof(size_t), source.size(), source.data());

        return size_written;
    }

    template <typename T> void Append(size_t count, const T* source) {
        cursor += Write<T>(cursor, count, source);
    }

    template <typename T> void Append(const std::vector<T>& source) {
        cursor += Write<T>(cursor, source);
    }

    template <typename T> void Append(const T& value) {
        cursor += Write<T>(cursor, value);
    }

    template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>> std::optional<T> Read() {
        if (cursor + sizeof(T) > data.size())
            return std::nullopt;

        T out{};

        std::memcpy(&out, data.data() + cursor, sizeof(T));
        cursor += sizeof(T);
        return out;
    }

    std::optional<std::string> ReadString() {
        auto size_opt = Read<size_t>();
        if (!size_opt)
            return std::nullopt;

        size_t size = size_opt.value();

        if (cursor + size > data.size())
            return std::nullopt;

        std::string out(" ", size);
        std::memcpy(out.data(), data.data() + cursor, size);

        cursor += size;
        return out;
    }

    template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
    std::optional<std::vector<T>> ReadVector() {
        auto size_opt = Read<size_t>();
        if (!size_opt)
            return std::nullopt;

        size_t size = size_opt.value();

        size_t arraySize = size * sizeof(T);

        if (cursor + arraySize > data.size())
            return std::nullopt;

        std::vector<T> out;
        out.resize(size);

        std::memcpy(out.data(), data.data() + cursor, arraySize);
        cursor += arraySize;

        return out;
    }

    bool operator==(const ByteArray& array);

    bool WriteToStream(Stream& stream) override;
    bool LoadFromStream(Stream& stream) override;

    size_t GetFullSize() {
        return sizeof(char) + sizeof(size_t) + data.size() * sizeof(byte);
    };

    void SetCursor(size_t value) {
        cursor = value;
    }
    size_t GetCursor() {
        return cursor;
    }

    const std::vector<byte>& GetData() const {
        return data;
    }
    std::vector<byte>& Vector() {return data;}
    byte* Data(){ return data.data(); }
    size_t Size() {return data.size(); }
};
