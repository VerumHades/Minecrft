#pragma once

#include <rendering/bitworks.hpp>
#include <unordered_set>

#define CHUNK_SIZE 64

enum LODLevel{
    CLOSE,
    MID,
    MID_FAR,
    FAR
};

class DynamicChunkMask{
    public:
        virtual uint_t<64> getAt(uint32_t z,uint32_t y) = 0;
        virtual uint_t<64> getRotatedAt(uint32_t x,uint32_t y) = 0;
        virtual bool get(uint32_t x,uint32_t y,uint32_t z) = 0;
        virtual Block& getBlock() = 0;
        virtual int getSize() = 0;
};

template <int size>
class ChunkMask: public DynamicChunkMask{
    public:
        Block block = {BlockTypes::Air};
        BitArray3D<size> segments = {}; 
        BitArray3D<size> segmentsRotated = {}; 

        ChunkMask(){}
        ChunkMask(Block block): block(block){}
        
        void set(uint32_t x,uint32_t y,uint32_t z) {
            segments[z][y] |= (1ULL << (size - 1 - x));
            segmentsRotated[x][y] |= (1ULL << (size - 1 - z));
        }
        void reset(uint32_t x,uint32_t y,uint32_t z) {
            segments[z][y] &= ~(1ULL << (size - 1 - x));
            segmentsRotated[x][y] &= ~(1ULL << (size - 1 - z));
        }
        bool get(uint32_t x,uint32_t y,uint32_t z) override {
            return segments[z][y] & (1ULL << (size - 1 - x));
        }

        uint_t<64> getAt(uint32_t z,uint32_t y) override { 
            return segments[z][y];
        };
        uint_t<64> getRotatedAt(uint32_t x,uint32_t y) override {
            return segmentsRotated[x][y];
        };

        Block& getBlock() override {
            return block;
        }

        int getSize() override {return size;}
};

struct DynamicChunkContents{
    virtual DynamicChunkMask& getSolidMask() = 0;
    virtual DynamicChunkMask& getMask(BlockTypes type) = 0;
    virtual bool hasMask(BlockTypes type) = 0;
    virtual std::unordered_set<BlockTypes> getMaskTypes() = 0;

    virtual int getSize() = 0;
    virtual bool empty() = 0;
};

template <int size>
struct ChunkMaskGroup: public DynamicChunkContents{
    ChunkMask<size> solidMask = {};
    std::unordered_map<BlockTypes,ChunkMask<size>> masks = {};
    
    std::unordered_set<BlockTypes> getMaskTypes() override {
        std::unordered_set<BlockTypes> keys;
        for (auto& [type, _] : masks) {
            keys.emplace(type); 
        }
        return keys;
    }
    DynamicChunkMask& getSolidMask() override { return solidMask;};
    DynamicChunkMask& getMask(BlockTypes type) override { return masks[type];};
    bool hasMask(BlockTypes type) override { return masks.count(type) != 0;}

    int getSize() override {return size;}
    bool empty() override {return masks.size() == 0;}
};