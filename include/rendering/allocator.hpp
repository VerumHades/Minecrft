#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <vector>
#include <iostream>
#include <functional>
#include <list>
#include <map>

using AllocatorMemoryRequest = std::function<bool(size_t)>;

class Allocator{
    private:
        size_t memsize;

        struct MemBlock;
        using MemBlockIterator = std::list<MemBlock>::iterator;

        struct MemBlock{
            size_t start;
            size_t size;
            
            bool used;
            std::multimap<size_t,MemBlockIterator>::iterator freeRegistery;
        };

        std::list<MemBlock> blocks = {};
        std::multimap<size_t,MemBlockIterator> freeBlocks = {};
        std::unordered_map<size_t,MemBlockIterator> takenBlocks = {};

        AllocatorMemoryRequest requestMemory;

        void markFree(MemBlockIterator block){
            block->used = false;
            block->freeRegistery = freeBlocks.insert({block->size,block});
        }

    public:
        Allocator(size_t memsize, AllocatorMemoryRequest requestMemory):  memsize(memsize), requestMemory(requestMemory) {
            markFree(blocks.insert(blocks.end(),{0, memsize, false}));
        };
        Allocator(){
            memsize = 0;
        }

        size_t allocate(size_t size);
        void free(size_t location);
        void clear();

        const std::list<MemBlock>& getBlocks() {return blocks;};
        const size_t& getMemorySize(){return memsize;}
};

#endif