#include <structure/bitworks.hpp>

using namespace bitworks;

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