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
        std::vector<T> data;
    public:
        AllocatedList(size_t initial_size){
            data.resize(initial_size);

            allocator = Allocator(initial_size, [this](size_t size){
                allocator.expand(size);
                data.resize(data.size() + size);

                return true;
            });        
        }

        /*
            If source is nullptr only allocates
        */
        size_t insert(T* source, size_t size){
            auto [success, start] = allocator.allocate(size);

            if(!success) return -1;
            if(source == nullptr) return start;

            auto destination = data.data() + start;
            std::memcpy(destination, source, size * sizeof(T));

            return start;
        }

        void free(size_t taken_start){
            allocator.free(taken_start);
        }

        std::vector<T>::iterator begin(){
            return data.begin();
        }

        std::vector<T>::iterator end(){
            return data.end();
        }

        T& operator[](std::size_t index) {
            return data[index];
        }

        const T& operator[](std::size_t index) const {
            return data[index];
        }

        T* data(){
            return data.data();
        }

        size_t size(){
            return data.size();
        }
};