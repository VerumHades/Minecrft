#include <rendering/allocator.hpp>

/*
    Allocates memory to the block of closest size, resizes blocks to be exactly the size of the allocation.
*/
template <typename T>
T* Allocator<T>::allocate(size_t size){
    size_t current_distance = (size_t)-1; // Max 
    
    MemBlock* selected = nullptr;
    int selected_index = -1;

    for(int i = 0; i < blocks.size(); i++){
        MemBlock& block = blocks[i];

        if(block.used) continue; // Block is allocated

        size_t distance = block.size > size ? block.size - size : size - block.size;

        if(distance >= current_distance) continue; // Block is not closer to the desired size

        selected = &block;
        selected_index = i;
        current_distance = distance;
    }

    if(selected == nullptr) return nullptr; // Failed to find block of desired size
    
    if(selected->size > size){ // Will fragment memory without regard, maybe update later?
        MemBlock newBlock;

        newBlock.start = selected->start + size;
        newBlock.size = selected->size - size; // Give it the size left;
        newBlock.used = false;

        blocks.insert(blocks.begin() + selected_index + 1, newBlock);
    }

    
    selected->size = size;
    selected->used = true;

    return memory + selected->start;
}   

template <typename T>
void Allocator<T>::free(T* ptr){
    size_t start = ptr - memory;

    for(auto& block: blocks){
        if(block.start != start) continue;

        if(!block.used){
            std::cout << "Double free in allocator!" << std::endl;
        }

        block.used = false;
        return;
    }

    std::cout << "Free of unlocatable block: " << start << std::endl;
}

template <typename T>
size_t Allocator<T>::getOffset(T* ptr){
    return ptr - memory;
}


template class Allocator<float>;          // Explicitly instantiate for float
template class Allocator<unsigned int>;   // Explicitly instantiate for unsigned int