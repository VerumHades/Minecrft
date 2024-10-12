#include <rendering/bitworks.hpp>

uint64_t operator"" _uint64(unsigned long long value) {
    return value;
}

inline uint8_t getMode(compressed_byte member){
    return (0b11000000 & member) >> 6;
}
inline uint8_t getCount(compressed_byte member){
    return 0b00111111 & member;
}
inline void setMode(compressed_byte& member, uint8_t mode){
    member |= (mode << 6);
}

uint64_t firstBitMask = (1_uint64 << 63);
/*
    Compresses a 64 bit uint into an array of 8 bits, will almost never go over the original size only in the worst case scenario of alternating bits.
    
    00-000000
    
    First two bits are for modes: 
        0. ZEROES
        1. ONES
        2. LITERAL 
        4. unused

    The remaining bits are a count max at 64  
*/
std::vector<compressed_byte> bitworks::compress64Bits(uint64 bits){
    std::vector<compressed_byte> out;
    out.reserve(10);
    uint8_t compressed_total = 0;

    while(compressed_total < 64){
        uint8_t start_value = (firstBitMask & bits) >> 63; // Check what the current first value is
        uint8_t count = count_leading_zeros(!start_value ? bits : ~bits);
        
        if(compressed_total + count > 64) count = 64 - compressed_total; // Dont go over 64 bit boundary

        if(count <= 8 && count >= 6){ // Compressing 8 bits or less into counts is not good, use a literal instead
            compressed_byte current = bits >> (64 - 6); // shift so that the first six bits are in the right place
            setMode(current, 2); // set the mode to literal

            //std::cout << std::bitset<64>(bits) << std::endl;
           // std::cout << std::bitset<8>(current) << std::endl;

            bits <<= 6; // Shift by the 6 literal bits
            compressed_total += 6;
            out.push_back(current);

            continue;
        }

        compressed_byte current = count - 1;
        setMode(current, start_value); // set the mode
        
        bits <<= count;
        compressed_total += count;
        out.push_back(current);
        

        //std::cout << static_cast<int>(count) << std::endl;
    }

    return out;
}
uint64 bitworks::decompress64Bits(std::vector<compressed_byte> bytes){
    uint64 out = 0;
    uint64 mask = ~0_uint64;

    uint8_t currentShift = 0;
    for(auto& value: bytes){
        switch (getMode(value))
        {
        case 0: // set zeroes
            out &= ~mask;
            break;
        case 1: // Set ones
            out |= mask;
            break;
        case 2: // Set literal
            out &= ~mask;
            out |= (static_cast<uint64_t>(getCount(value)) << ((64 - 6) - currentShift));
            mask >>= 6;
            currentShift += 6;
            continue;
        default:
            break;
        }

        uint8_t count = getCount(value) + 1;
        if(count != 0 && count != 64) mask >>= count; 
        currentShift += count; 
    }

    return out;
}

static uint32_t valueMask = ~0U >> 2;
void compressed_24bit::setValue(uint32_t value){
    uint8_t mode = getMode();

    bytes[0] = value;
    bytes[1] = value >> 8;
    bytes[2] = value >> 16; // Leave mode  bits untouched

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
std::vector<compressed_24bit> bitworks::compressBitArray3D(BitArray3D array){
    std::vector<compressed_24bit> compressedOutput = {};
    uint64* flatArray = array.getAsFlatArray();
    
    uint64 currentBits = flatArray[0];

    size_t currentIndex = 0;
    size_t localBit = 0;
    size_t globalBit = 0;

    while(globalBit < BitArray3D::size_bits){
        uint8_t start_value = (firstBitMask & currentBits) >> 63; // Check what the current first value is
        uint8_t count = count_leading_zeros(!start_value ? currentBits : ~currentBits);

        if(localBit + count > 64) count = 64 - localBit;

        currentBits <<= count;
        localBit    +=  count;
        globalBit   +=  count;

        if(localBit == 64){ // Move on to the next number
            localBit = 0;
            currentBits = flatArray[++currentIndex];
        }

        if(compressedOutput.size() != 0){
            compressed_24bit& last = compressedOutput[compressedOutput.size() - 1];
            if(start_value == last.getMode()){ // If possible save the same bits in the last compressed_24bit
                last.setValue(last.getValue() + count);
                //last.setMode(last.getMode());
                continue;
            }
        }

        compressed_24bit compressed = {};
        compressed.setValue(count);
        compressed.setMode(start_value);
        
        compressedOutput.push_back(compressed);
    }   

    return compressedOutput;
}

BitArray3D bitworks::decompressBitArray3D(std::vector<compressed_24bit> data){
    BitArray3D output;
    uint64* flatArray = output.getAsFlatArray();

    size_t arrayIndex = 0;
    size_t dataIndex  = 0;
    size_t currentBit = 0;
    
    compressed_24bit cdata = data[0];
    while(true){
        size_t bitsLeft = 64 - currentBit; // Find the remaining amount of bits in the current 64bit
        
        if(bitsLeft == 0){ // If no bits are left move onto the next 
            if(arrayIndex + 1 >= BitArray3D::size) break; // If there is no next number exit
            currentBit = 0;
            arrayIndex++;
            continue;
        }
        //if(cdata.getValue() < 1024) std::cout << cdata.to_string() << std::endl;

        size_t count = std::min(bitsLeft, static_cast<size_t>(cdata.getValue()));
        
        uint64 mask = ~0_uint64 >> currentBit;
            
        if(cdata.getMode() == 0) flatArray[arrayIndex] &= ~mask;
        else flatArray[arrayIndex] |= mask;

        currentBit += count;

        cdata.setValue(cdata.getValue() - count);
        if(cdata.getValue() == 0){ // If the current compressed 24bit is completed move onto the next
            if(dataIndex + 1 >= data.size()) break; // break if there is no next

            cdata = data[++dataIndex];
        }
    }

    /*for(int i = 0;i < BitArray3D::size;i++){
        std::cout << std::bitset<64>(flatArray[i]) << std::endl;
    }*/

    return output;
}

void ByteArray::write(std::fstream &file){
    bitworks::saveValue<char>(file,'|'); // Magic start character
    //std::cout << data.size() << std::endl;
    bitworks::saveValue<size_t>(file, data.size());
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void ByteArray::read(std::fstream &file){
    if(bitworks::readValue<char>(file) != '|'){ // Check for magic start character
        std::cout << "Invalid start of byte array." << std::endl;
        return;
    }
    size_t size = bitworks::readValue<size_t>(file);
    data.resize(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
}

bool ByteArray::operator== (const ByteArray& array){
    if(data.size() != array.data.size()) return false;
    return std::memcmp(data.data(), array.data.data(), data.size()) == 0;
}