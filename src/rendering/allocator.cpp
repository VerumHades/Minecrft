#include <rendering/allocator.hpp>


Allocator::Allocator(size_t memsize, std::function<bool(size_t)>&& requestMemory) :  memsize(memsize), requestMemory(requestMemory) {
    reset(memsize);
};
/*
    Allocates memory to the block of closest size, resizes blocks to be exactly the size of the allocation.
*/
std::tuple<bool,size_t> Allocator::allocate(size_t size){
    auto it = freeBlocks.lower_bound(size);

    if(it == freeBlocks.end()){
        //std::cout << "Couldnt allocate: " << size << std::endl;
        //std::cout << "Allocated at:" << selectedBlock->start << " of size: " << size << std::endl;
        if(requestMemory && requestMemory(size)){ // Failed to find block of desired size
            return allocate(size);
        }
        else{
            return {false, 0};
        }
    }

    auto [actual_size, selectedBlock] = *it;
    freeBlocks.erase(it);
    
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
    return {true, selectedBlock->start};
}   

bool Allocator::free(size_t start, std::string fail_prefix){
    if(!takenBlocks.contains(start)) {
        std::cout << fail_prefix << ": "<< "Free of unalocated block: " << start << std::endl;
        return false;
    }
    auto block = takenBlocks.at(start);
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
    return true;
}


/*bool Allocator::splitTakenBlock(size_t at, const std::vector<size_t>& sizes){
    // Make sure the block exists
    if(!takenBlocks.contains(at)) {
        std::cerr << "Cannot split non-existing block: " << at << std::endl;
        return false;
    }
    auto block = takenBlocks.at(at);
    
    // Make sure the sizes are valid
    size_t counted_size_total = 0;
    for(auto& size: sizes) counted_size_total += size;

    if(counted_size_total != block->size){
        std::cerr << "Split sizes dont align with the total size of the block." << at << std::endl;
        return false;
    }

    size_t current_position = at;
    
    std::cout << "New subblock at: " << current_position << " of size: " << sizes[0] << std::endl;

    block->size = sizes[0]; // Resize the current block
    current_position += block->size;

    auto current_block = block;
    for(int i = 1;i < sizes.size();i++){
        size_t size = sizes[i];
        size_t position = current_position;

        std::cout << "New subblock at: " << position << " of size: " << size << std::endl;

        current_block = blocks.insert(std::next(current_block), {position, size, true});
        takenBlocks[position] = current_block;

        current_position += size;
    }

    return true;
}*/

size_t Allocator::getTakenBlockSize(size_t at){
    if(!takenBlocks.contains(at)) return 0;
    return takenBlocks.at(at)->size;
}