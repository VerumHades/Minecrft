#include <rendering/allocator.hpp>

/*
    Allocates memory to the block of closest size, resizes blocks to be exactly the size of the allocation.
*/
size_t Allocator::allocate(size_t size){
    auto it = freeBlocks.lower_bound(size);

    if(it == freeBlocks.end()){
        std::cout << "Couldnt allocate: " << size << std::endl;
        //std::cout << "Allocated at:" << selectedBlock->start << " of size: " << size << std::endl;
        if(requestMemory(size)){ // Failed to find block of desired size
            return allocate(size);
        }
        else{
            return 0;
        }
    }

    freeBlocks.erase(it);
    auto [actual_size, selectedBlock] = *it;


    size_t sizeLeft = selectedBlock->size - size;
    if(selectedBlock->size > size){ // Defragmented when freeing
        MemBlock newBlock;

        newBlock.start = selectedBlock->start + size;
        newBlock.size = sizeLeft; // Give it the size left;

        auto iter = blocks.insert(std::next(selectedBlock), newBlock);
        markFree(iter);
        //std::cout << "Resized memory block: " << size << " New block: " << newBlock.size << std::endl;
    }

    selectedBlock->used = true;
    selectedBlock->size = size;
    takenBlocks[selectedBlock->start] = selectedBlock;
    //std::cout << "Allocated at:" << selectedBlock->start << " of size: " << size << std::endl;
    //std::cout << "Current state:" << std::endl;
    //for(auto& block: blocks){
    //    std::cout << "(" << block.start << ":" << block.size << ")[" << (block.used ? "used" : "unused") << "] ";
    //}
    //std::cout << std::endl;
    return selectedBlock->start;
}   

void Allocator::clear(){
    blocks = {};
    freeBlocks = {};
    takenBlocks = {};

    markFree(blocks.insert(blocks.end(),{0, memsize, false}));
}

void Allocator::free(size_t start){
    if(takenBlocks.count(start) == 0) {
        std::cout << "Free of unlocatable block: " << start << std::endl;
        return;
    }
    auto& block = takenBlocks[start];
    takenBlocks.erase(start);
    
    if(std::next(block) != blocks.end()){
        auto nextBlock = std::next(block);
        if(!nextBlock->used){
            block->size += nextBlock->size;
            freeBlocks.erase(nextBlock->freeRegistery);
            blocks.erase(nextBlock);
        }
    }
    else if(block != blocks.begin()){
        auto previousBlock = std::prev(block);
        if(!previousBlock->used){
            block->start = previousBlock->start;
            block->size += previousBlock->size;

            freeBlocks.erase(previousBlock->freeRegistery);
            blocks.erase(previousBlock);
        }
    }

    markFree(block);
}