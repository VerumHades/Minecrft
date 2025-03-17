#include <structure/bitworks.hpp>

using namespace bitworks;

template <typename T>
inline void saveToBuffer(std::vector<byte>& buffer, const T& value){
    const byte* start = reinterpret_cast<const byte*>(&value);
    buffer.insert(buffer.end(), start, start + sizeof(T));
}

void ByteArray::write(std::fstream &file, bool headless){
    if(!headless){
        bitworks::saveValue<char>(file,'|');
        bitworks::saveValue<size_t>(file, data.size());
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void ByteArray::writeToBuffer(std::vector<byte>& buffer){
    saveToBuffer<char>(buffer, '|');
    saveToBuffer<size_t>(buffer, data.size());
    buffer.insert(buffer.end(), data.begin(), data.end());
}

bool ByteArray::read(std::fstream &file){
    char value = bitworks::readValue<char>(file);
    if(value != '|'){ // Check for magic start character
        return false;
    }
    size_t size = bitworks::readValue<size_t>(file);
    data.resize(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return true;
}

bool ByteArray::operator== (const ByteArray& array){
    if(data.size() != array.data.size()) return false;
    return std::memcmp(data.data(), array.data.data(), data.size()) == 0;
}

bool ByteArray::saveToFile(fs::path path){
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file) {
        LogError("Failed to open file '{}'.", path.string());
        return false;
    }

    write(file);
    return true;
}
ByteArray ByteArray::FromFile(fs::path path){
    std::fstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        LogError("Failed to open file '{}'.", path.string());
        return {};
    }
    
    ByteArray output{};
    output.read(file);

    return output;
}

ByteArray ByteArray::FromStream(std::fstream &file){
    ByteArray output{};
    output.read(file);
    return output;
}

BlockBitPlanes::BlockBitPlanes(){
    planes = std::vector<BitPlane<64>>(BlockRegistry::get().registeredBlocksTotal());
}

void BlockBitPlanes::setRow(size_t type, size_t row, uint64_t value){
    if(log && value != 0){
        std::cout << "For row: " << row << " with block ID: " << type << "\n";
        std::cout << "Value: " << std::bitset<64>(value) << "\n";
        std::cout << "Mask : " << std::bitset<64>(mask[row]) << "\n";
    }
    value = value & (~mask[row]);
    mask[row] |= value;
    planes[type][row] = value;
}