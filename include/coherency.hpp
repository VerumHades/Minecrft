#pragma once

#include <rendering/allocator.hpp>
#include <cstring>
/*
    List that remains cache friendly
*/
template <typename T>
class AllocatedList{
    private:
        Allocator allocator;
        std::vector<T> internal_vector;
    public:
        AllocatedList(size_t initial_size){
            internal_vector.resize(initial_size);

            allocator = Allocator(initial_size, [this](size_t size){
                allocator.expand(size);
                internal_vector.resize(internal_vector.size() + size);

                std::cout << "Expaned: " << internal_vector.size() << " " << size << std::endl;
                return true;
            });        
        }

        /*
            If source is nullptr only allocates
        */
        size_t insert(T* source, size_t size){
            auto [success, start] = allocator.allocate(size);

            if(!success) return -1ULL;
            if(source == nullptr) return start;

            auto destination = internal_vector.data() + start;
            std::memcpy(destination, source, size * sizeof(T));

            return start;
        }

        void free(size_t taken_start){
            allocator.free(taken_start);
        }

        std::vector<T>::iterator begin(){
            return internal_vector.begin();
        }

        std::vector<T>::iterator end(){
            return internal_vector.end();
        }

        T& operator[](std::size_t index) {
            return internal_vector[index];
        }

        const T& operator[](std::size_t index) const {
            return internal_vector[index];
        }

        T* data(){
            return internal_vector.data();
        }

        size_t size(){
            return internal_vector.size();
        }
};