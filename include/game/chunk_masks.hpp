#pragma once

#include <rendering/bitworks.hpp>

#define CHUNK_SIZE 64

enum LODLevel{
    CLOSE,
    MID,
    MID_FAR,
    FAR
};

template <int size>
class ChunkMask{
    public:
        Block block = {BlockTypes::Air};
        BitArray3D<size> segments = {}; 
        BitArray3D<size> segmentsRotated = {}; 
        
        void set(uint32_t x,uint32_t y,uint32_t z) {
            segments[z][y] |= (1ULL << (size - 1 - x));
            segmentsRotated[x][y] |= (1ULL << (size - 1 - z));
        }
        void reset(uint32_t x,uint32_t y,uint32_t z) {
            segments[z][y] &= ~(1ULL << (size - 1 - x));
            segmentsRotated[x][y] &= ~(1ULL << (size - 1 - z));
        }
};

struct DynamicChunkMaskGroup{
    virtual int getSize() = 0;
    virtual bool empty() = 0;
};

template <int size>
struct ChunkMaskGroup: public DynamicChunkMaskGroup{
    ChunkMask<size> solidMask = {};
    std::unordered_map<BlockTypes,ChunkMask<size>> masks = {};

    int getSize() override {return size;}
    bool empty() override {return masks.size() == 0;}
};