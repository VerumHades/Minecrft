#pragma once

#include <rendering/bitworks.hpp>
#include <unordered_set>

#define CHUNK_SIZE 64

class DynamicChunkMask{
    private:
        Block block = {BlockTypes::Air};
        DynamicBitArray3D segments; 
        DynamicBitArray3D segmentsRotated; 

    public:
        DynamicChunkMask(size_t size, Block block): segments(size), segmentsRotated(size), block(block) {}
        DynamicChunkMask(size_t size, CompressedArray segments_compressed): segments(size, segments_compressed), segmentsRotated(size){
            this->segmentsRotated.loadAsRotated(segments);
        }
        DynamicChunkMask(size_t size, DynamicBitArray3D array): segments(size), segmentsRotated(size){
            segmentsRotated.loadAsRotated(segments);
        }
        
        void set(uint32_t x,uint32_t y,uint32_t z) {
            segments.setBit(z,y,x);
            segmentsRotated.setBit(x,y,z);
        }
        void reset(uint32_t x,uint32_t y,uint32_t z) {
            segments.resetBit(z,y,x);
            segmentsRotated.resetBit(x,y,z);
        }
        bool get(uint32_t x,uint32_t y,uint32_t z) {
            return segments.getBit(x,y,z);
        }

        uint_t<64> getAt(uint32_t z,uint32_t y) { 
            return segments.getRow(z, y);
        };
        uint_t<64> getRotatedAt(uint32_t x,uint32_t y) {
            return segmentsRotated.getRow(x, y);
        };

        Block& getBlock(){
            return block;
        }
        void setBlock(Block block){
            this->block = block;
        }

        size_t getSize() {return segments.getSize();}

        DynamicBitArray3D& getSegments() { return segments; };
};

class DynamicChunkContents{
    private:
        DynamicChunkMask solidMask;
        std::unordered_map<BlockTypes,DynamicChunkMask> masks = {};
    
    public:
        DynamicChunkContents(size_t size): solidMask(size, BlockTypes::Air) {}

        std::unordered_set<BlockTypes> getMaskTypes() {
            std::unordered_set<BlockTypes> keys;
            for (auto& [type, _] : masks) {
                keys.emplace(type); 
            }
            return keys;
        }
        DynamicChunkMask& getMask(BlockTypes type) { return masks.at(type);};
        bool hasMask(BlockTypes type) { return masks.count(type) != 0;}

        std::unordered_map<BlockTypes,DynamicChunkMask>& getMasks() {return masks;};
        DynamicChunkMask& getSolidMask() {return solidMask;}
        void setSolidMask(DynamicChunkMask& mask) { solidMask = mask;}
        void setMask(BlockTypes type, DynamicChunkMask& mask) {masks.insert_or_assign(type,mask);}
        void createMask(BlockTypes  type, int size) {DynamicChunkMask mask = {size,type}; setMask(type, mask);}

        size_t getSize() {return solidMask.getSize();}
        bool empty() {return masks.size() == 0;}
};