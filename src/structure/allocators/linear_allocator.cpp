#include <structure/allocators/linear_allocator.hpp>

void LinearAllocator::Allocate(size_t size){
    size_t start = end;
    end += size;
    return start;
}
size_t LinearAllocator::End(){
    return end;
}
