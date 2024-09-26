#include <rendering/allocator.hpp>

/*
    Allocates memory to the block of closest size, resizes blocks to be exactly the size of the allocation.
*/
size_t Allocator::allocate(size_t size){
    size_t current_distance = (size_t)-1; // Max 
    int selected_index = -1;

    for(int i = 0; i < blocks.size(); i++){
        MemBlock& block = blocks[i];

        if(block.used) continue; // Block is allocated

        size_t distance = block.size > size ? block.size - size : size - block.size;
        if(distance >= current_distance) continue; // Block is not closer to the desired size

        selected_index = i;
        current_distance = distance;
    }

    
    if(selected_index == -1){
        std::cout << "Couldnt allocate: " << size << std::endl;
        std::cout << "Allocated at:" << blocks[selected_index].start << " of size: " << size << std::endl;
        std::cout << "Current state:" << std::endl;
        for(auto& block: blocks){
            std::cout << "(" << block.start << ":" << block.size << ")[" << (block.used ? "used" : "unused") << "] ";
        }
        std::cout << std::endl;
        return 0; // Failed to find block of desired size
    }
    
    if(blocks[selected_index].size > size){ // Will fragment memory without regard, maybe update later?
        MemBlock newBlock;

        newBlock.start = blocks[selected_index].start + size;
        newBlock.size = blocks[selected_index].size - size; // Give it the size left;
        newBlock.used = false;

        blocks.insert(blocks.begin() + selected_index + 1, newBlock);
        std::cout << "Resized memory block: " << size << " New block: " << newBlock.size << std::endl;
    }

    blocks[selected_index].size = size;
    blocks[selected_index].used = true;

    //std::cout << "Allocated at:" << blocks[selected_index].start << " of size: " << size << std::endl;
    //std::cout << "Current state:" << std::endl;
    //for(auto& block: blocks){
    //    std::cout << "(" << block.start << ":" << block.size << ")[" << (block.used ? "used" : "unused") << "] ";
    //}
    //std::cout << std::endl;

    return blocks[selected_index].start;
}   

void Allocator::free(size_t start){
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