#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <vector>
#include <iostream>

template <typename T>
class Allocator{
    private:
        size_t memsize;
        T* memory;

        struct MemBlock{
            size_t start;
            size_t size;
            
            bool used;
        };

        std::vector<MemBlock> blocks;

    public:
        Allocator(T* memory, size_t memsize) : memory(memory), memsize(memsize) {
            blocks.push_back(MemBlock(0,memsize));
        };

        T* allocate(size_t size);
        void free(T* prt);
        size_t getOffset(T* ptr);
};

#endif