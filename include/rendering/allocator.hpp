#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <vector>
#include <iostream>

class Allocator{
    private:
        size_t memsize;

        struct MemBlock{
            size_t start;
            size_t size;
            
            bool used;
        };

        std::vector<MemBlock> blocks;

    public:
        Allocator(size_t memsize):  memsize(memsize) {
            blocks.push_back({0, memsize, false});
        };
        Allocator(){
            memsize = 0;
        }

        size_t allocate(size_t size);
        void free(size_t location);
};

#endif