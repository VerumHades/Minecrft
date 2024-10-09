#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <vector>
#include <iostream>
#include <functional>

using AllocatorMemoryRequest = std::function<bool(size_t)>;

class Allocator{
    private:
        size_t memsize;

        struct MemBlock{
            size_t start;
            size_t size;
            
            bool used;
        };

        struct FreeBlock{
            size_t index;
            size_t size;
        };

        std::vector<MemBlock> blocks;
        std::vector<FreeBlock> freeBlocks;

        AllocatorMemoryRequest requestMemory;

    public:
        Allocator(size_t memsize, AllocatorMemoryRequest requestMemory):  memsize(memsize), requestMemory(requestMemory) {
            blocks.push_back({0, memsize, false});
            freeBlocks.push_back({0, memsize});
        };
        Allocator(){
            memsize = 0;
        }

        size_t allocate(size_t size);
        void free(size_t location);
        void clear();
        size_t appendBlock(size_t size){
            blocks.push_back({memsize, size, false});
            memsize += size;
            return memsize - size;
        }

        size_t getSize(){return memsize;}
};

#endif