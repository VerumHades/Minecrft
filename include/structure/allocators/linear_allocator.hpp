#pragma once

class LinearAllocator {
    private:
        size_t end = 0;
    public:
        void Allocate(size_t size);
        size_t End();
}
