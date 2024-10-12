#include <game/world_saving.hpp>

template <typename T>
static inline void saveValue(std::ofstream &file, T value){
    file.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
static inline T readValue(std::ifstream &file) {
    T value;
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
}

void ByteArray::write(std::ofstream &file){
    saveValue<char>(file,'|'); // Magic start character
    std::cout << data.size() << std::endl;
    saveValue<size_t>(file, data.size());
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void ByteArray::read(std::ifstream &file){
    if(readValue<char>(file) != '|'){ // Check for magic start character
        std::cout << "Invalid start of byte array." << std::endl;
    }
    size_t size = readValue<size_t>(file);
    data.resize(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
}

bool ByteArray::operator== (const ByteArray& array){
    if(data.size() != array.data.size()) return false;
    return std::memcmp(data.data(), array.data.data(), data.size()) == 0;
}