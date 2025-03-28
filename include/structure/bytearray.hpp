#pragma once

#include <cstring>
#include <cstdint>
#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <fstream>
#include <optional>

#include <structure/streams/stream.hpp>
#include <structure/streams/streamable.hpp>

#include <path_config.hpp>
#include <logging.hpp>

class ByteArray: Streamable{
    private:
        std::vector<byte> data = {};
        size_t cursor = 0;
    public:    
        ByteArray(){}
        bool append(const ByteArray& array){
            data.insert(data.end(), array.data.begin(), array.data.end());
            return true;
        }
      
        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        bool append(const T& data){
            auto* bytePtr = reinterpret_cast<const byte*>(&data);
            this->data.insert(this->data.end(), bytePtr, bytePtr + sizeof(T));
            return true;
        }
        
        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        bool append(const std::vector<T>& source){
            size_t totalSize = source.size() * sizeof(T);
            append<size_t>(source.size()); 

            data.resize(data.size() + totalSize);
            auto* sourceArray = reinterpret_cast<const byte*>(source.data());
            std::memcpy(data.data() + data.size() - totalSize, sourceArray, totalSize);

            return true;
        }

        bool append(const std::string& source){
            size_t totalSize = source.size();
            append<size_t>(source.size()); 

            data.resize(data.size() + totalSize);
            auto* sourceArray = reinterpret_cast<const byte*>(source.data());
            std::memcpy(data.data() + data.size() - totalSize, sourceArray, totalSize);

            return true;
        }

        std::optional<std::string> sread(){
            auto size_opt = read<size_t>();
            if(!size_opt) return std::nullopt;
            size_t size = size_opt.value();

            if(cursor + size > data.size()) return std::nullopt;
            
            std::string out(" ",size);
            std::memcpy(out.data(), data.data() + cursor, size);

            cursor += size;
            return out;
        }

        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        std::optional<T> read(){
            if(cursor + sizeof(T) > data.size()) return std::nullopt;
            
            T out{};

            std::memcpy(&out, data.data() + cursor, sizeof(T));
            cursor += sizeof(T);
            return out;
        }

        template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        std::optional<std::vector<T>> vread(){
            auto size_opt = read<size_t>();
            if(!size_opt) return std::nullopt;
            size_t size = size_opt.value();

            size_t arraySize = size * sizeof(T);

            if(cursor + arraySize > data.size()) return std::nullopt;

            std::vector<T> out;
            out.resize(size);

            std::memcpy(out.data(), data.data() + cursor, arraySize);
            cursor += arraySize;

            return out;
        }

        bool operator == (const ByteArray& array);
        
        bool WriteToStream(Stream& stream) override;
        bool LoadFromStream(Stream& stream) override;

        size_t GetFullSize() {return sizeof(char) + sizeof(size_t) + data.size() * sizeof(byte);};

        const std::vector<byte>& GetData() const {return data;}
};