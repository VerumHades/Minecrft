#include <rendering/bitworks.hpp>

template <int bits>
CompressedArray bitworks::compress(uint_t<bits>* flatArray, size_t size){
    std::vector<compressed_24bit> compressedOutput = {};

    const size_t size_bits = size * bits;
    const uint_t<bits> firstBitMask = (1ULL << (bits - 1));

    uint_t<bits> currentBits = flatArray[0];

    size_t currentIndex = 0;
    size_t localBit = 0;
    size_t globalBit = 0;

    while(globalBit < size_bits){
        uint8_t start_value = (firstBitMask & currentBits) >> (bits - 1); // Check what the current first value is
        uint8_t count = count_leading_zeros<uint_t<bits>>(!start_value ? currentBits : ~currentBits);

        if(localBit + count > bits) count = bits - localBit;

        currentBits <<= count;
        localBit    +=  count;
        globalBit   +=  count;

        if(localBit == bits){ // Move on to the next number
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

    return {compressedOutput, size};
}

template <int bits>
uint_t<bits>* bitworks::decompress(CompressedArray compressed_data){
    const size_t size = compressed_data.source_size;

    uint_t<bits>* flatArray = new uint_t<bits>[size];

    size_t arrayIndex = 0;
    size_t dataIndex  = 0;
    size_t currentBit = 0;
    
    compressed_24bit cdata = compressed_data.data[0];
    while(true){
        size_t bitsLeft = bits - currentBit; // Find the remaining amount of bits in the current 64bit
        
        if(bitsLeft == 0){ // If no bits are left move onto the next 
            if(arrayIndex + 1 >= size) break; // If there is no next number exit
            currentBit = 0;
            arrayIndex++;
            continue;
        }
        //if(cdata.getValue() < 1024) std::cout << cdata.to_string() << std::endl;

        size_t count = std::min(bitsLeft, static_cast<size_t>(cdata.getValue()));
        
        uint_t<bits> mask = ~0ULL >> currentBit;
            
        if(cdata.getMode() == 0) flatArray[arrayIndex] &= ~mask;
        else flatArray[arrayIndex] |= mask;

        currentBit += count;

        cdata.setValue(cdata.getValue() - count);
        if(cdata.getValue() == 0){ // If the current compressed 24bit is completed move onto the next
            if(dataIndex + 1 >= compressed_data.data.size()) break; // break if there is no next

            cdata = compressed_data.data[++dataIndex];
        }
    }

    return flatArray;
}