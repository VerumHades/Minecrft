#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <vector>
#include <iostream>
#include <functional>
#include <list>
#include <map>
#include <queue>

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
        
        std::function<bool(size_t)> requestMemory;

        void markFree(MemBlockIterator block){
            block->used = false;
            block->freeRegistery = freeBlocks.insert({block->size,block});
        }

    public:
        Allocator(size_t memsize, std::function<bool(size_t)> requestMemory):  memsize(memsize), requestMemory(requestMemory) {
            reset(memsize);
        };
        Allocator(){
            memsize = 0;
        }

        void reset(size_t memsize){
            blocks = {};
            freeBlocks = {};
            takenBlocks = {};

            this->memsize = memsize;
            markFree(blocks.insert(blocks.end(),{0, memsize, false}));
        }

        void expand(size_t amount){
            if(!blocks.back().used) blocks.back().size += amount;
            else markFree(blocks.insert(blocks.end(),{memsize, amount, false}));
            
            memsize += amount;
        }

        std::tuple<bool,size_t> allocate(size_t size);
        bool free(size_t location, std::string fail_prefix = "");
        void clear(){
            reset(memsize);
        }

        /*
            Splits a block into subblocks
        */
        bool splitTakenBlock(size_t at, const std::vector<size_t>& sizes);

        /*
            Returns the size of a taken block, if the position is invalid returns 0
        */
        size_t getTakenBlockSize(size_t at);

        const std::list<MemBlock>& getBlocks() const {return blocks;};
        const std::unordered_map<size_t,MemBlockIterator>& getTakenBlocks() const {return takenBlocks;}
        const size_t& getMemorySize() const {return memsize;}
};

class PoolAllocator{
    private:
        std::queue<size_t> freeLocations = {};
        size_t memsize = 0;

    public:
        PoolAllocator(): PoolAllocator(0){}
        PoolAllocator(size_t memsize){
            for(int i = 0;i < memsize;i++) freeLocations.push(i);
        }
        // Allocates the first empty block
        std::tuple<bool,size_t> allocate(){
            if(freeLocations.empty()) return {false,0};
            size_t loc = freeLocations.front();
            freeLocations.pop();
            return {true,loc};
        }
        // Does no checks to validate the free, can cause issues
        void free(size_t ptr){
            freeLocations.push(ptr);
        }

        void increaseSize(size_t size){
            for(int i = 0; i < size;i++) freeLocations.push(memsize + i);
            memsize += size;
        }

        void clear(){
            freeLocations = {};
            for(int i = 0;i < memsize;i++) freeLocations.push(i);
        }

        size_t getMemSize() {return memsize;}
};

#endif