#include <rendering/bitworks.hpp>

using namespace bitworks;


void ByteArray::write(std::fstream &file){
    bitworks::saveValue<char>(file,'|'); // Magic start character
    //std::cout << data.size() << std::endl;
    bitworks::saveValue<size_t>(file, data.size());
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
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

bool ByteArray::saveToFile(std::string path){
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file) {
        std::cerr << "Failed to open file '" << path << "'." << std::endl;
        return false;
    }

    write(file);
    return true;
}
ByteArray ByteArray::FromFile(std::string path){
    std::fstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file '" << path << "'." << std::endl;
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