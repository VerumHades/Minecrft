#include <rendering/bitworks.hpp>

template <typename T>
std::vector<compressed_24bit> bitworks::compressBitArray3D(BitArray3D<T> array){
    std::vector<compressed_24bit> compressedOutput = {};

    T firstBitMask = (1_uint64 << BitArray3D<T>::T_bits_total - 1);

    T* flatArray = array.getAsFlatArray();
    T currentBits = flatArray[0];

    size_t currentIndex = 0;
    size_t localBit = 0;
    size_t globalBit = 0;

    while(globalBit < BitArray3D<T>::size_bits){
        uint8_t start_value = (firstBitMask & currentBits) >> (BitArray3D<T>::T_bits_total - 1); // Check what the current first value is
        uint8_t count = count_leading_zeros(!start_value ? currentBits : ~currentBits);

        if(localBit + count > BitArray3D<T>::T_bits_total) count = BitArray3D<T>::T_bits_total - localBit;

        currentBits <<= count;
        localBit    +=  count;
        globalBit   +=  count;

        if(localBit == BitArray3D<T>::T_bits_total){ // Move on to the next number
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

template <typename T>
BitArray3D<T> bitworks::decompressBitArray3D(std::vector<compressed_24bit> data){
    BitArray3D<T> output;
    T* flatArray = output.getAsFlatArray();

    size_t arrayIndex = 0;
    size_t dataIndex  = 0;
    size_t currentBit = 0;
    
    compressed_24bit cdata = data[0];
    while(true){
        size_t bitsLeft = BitArray3D<T>::T_bits_total - currentBit; // Find the remaining amount of bits in the current 64bit
        
        if(bitsLeft == 0){ // If no bits are left move onto the next 
            if(arrayIndex + 1 >= BitArray3D<T>::size) break; // If there is no next number exit
            currentBit = 0;
            arrayIndex++;
            continue;
        }
        //if(cdata.getValue() < 1024) std::cout << cdata.to_string() << std::endl;

        size_t count = std::min(bitsLeft, static_cast<size_t>(cdata.getValue()));
        
        T mask = ~0_uint64 >> currentBit;
            
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