#include <rendering/bitworks.hpp>

using namespace bitworks;

static uint32_t valueMask = ~0U >> 2;
void compressed_24bit::setValue(uint32_t value){
    uint8_t mode = getMode();

    bytes[0] = value;
    bytes[1] = value >> 8;
    bytes[2] = value >> 16;

    setMode(mode);
}
uint32_t compressed_24bit::getValue(){
    uint32_t value = 0;

    value |= static_cast<uint32_t>(bytes[2] & 0b00111111) << 16; // Make  sure to not consider the mode bytes
    value |= static_cast<uint32_t>(bytes[1]) << 8;
    value |= static_cast<uint32_t>(bytes[0]);

    value &= valueMask; // Remove the mode bits

    return value;
}
void compressed_24bit::setMode(uint8_t mode){
    bytes[2] &= 0b00111111; // Reset the mode bits
    bytes[2] |= (mode << 6);
}
uint8_t compressed_24bit::getMode(){
    return bytes[2] >> 6;
}

std::string compressed_24bit::to_string(){
    return 
        std::bitset<2>(bytes[2] >> 6).to_string() + "[" + ((bytes[2] >> 6) ? "ONES  " : "ZEROES") + "]-" +
        std::bitset<6>(bytes[2]).to_string() +
        std::bitset<8>(bytes[1]).to_string() +
        std::bitset<8>(bytes[0]).to_string() + "[" + std::to_string(getValue()) + "]";
}

/*

*/
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