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
        DynamicChunkMask(size_t size, CompressedArray segments): segments(size), segmentsRotated(size){
            this->segments = DynamicBitArray3D(size, segments);
            this->segmentsRotated = this->segments.getRotated();
        }
        
        void set(uint32_t x,uint32_t y,uint32_t z) {
            segments.set(z,y,x);
            segmentsRotated.set(x,y,z);
        }
        void reset(uint32_t x,uint32_t y,uint32_t z) {
            segments.reset(z,y,x);
            segmentsRotated.reset(x,y,z);
        }
        bool get(uint32_t x,uint32_t y,uint32_t z) {
            return segments.get(x,y,z);
        }

        uint_t<64> getAt(uint32_t z,uint32_t y) { 
            return segments.get(z, y);
        };
        uint_t<64> getRotatedAt(uint32_t x,uint32_t y) {
            return segmentsRotated.get(x, y);
        };

        Block& getBlock(){
            return block;
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
        void setSolidMask(DynamicChunkMask mask) { solidMask = mask;}
        void setMask(BlockTypes type, DynamicChunkMask mask) {masks.emplace(type,mask);}

        size_t getSize() {return solidMask.getSize();}
        bool empty() {return masks.size() == 0;}
};