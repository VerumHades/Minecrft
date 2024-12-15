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

                return true;
            });        
        }

        /*
            If source is nullptr only allocates
        */
        size_t insert(T* source, size_t size){
            if(size == 0) return 0;
            
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

/*
    A list that keeps all its contents tighly packed, uses a system of virtual regions to keep track of contents
*/
template <typename T>
class CoherentList{
    private:
        struct Region{
            size_t start = 0;
            size_t size  = 0;
        };

        std::list<Region> regions = {};
        
        T* internal_data = nullptr;
        size_t internal_size = 1;

        size_t content_size = 0;

    public:
        CoherentList(){
            internal_data = new T[internal_size];
        }
        ~CoherentList(){
            if(internal_data) delete internal_data;
        }

        const std::list<Region>::iterator append(T* data, size_t size){
            std::list<Region>::iterator region_iter = 
                regions.insert(regions.end(), {content_size, size});

            if(internal_size < content_size + size){
                T* old_data = internal_data;
                
                internal_size *= 2;
                internal_data = new T[internal_size];

                std::memcpy(internal_data, old_data, content_size * sizeof(T));

                delete old_data;
            }

            std::memcpy(internal_data + content_size, data, size * sizeof(T));
            content_size += size;
        }

        void remove(const std::list<Region>::iterator region){
            for (auto it = region; it != regions.end(); ++it) it->start -= region->size;
            std::memmove(
                internal_data + region->start,
                internal_data + region->start + region->size,
                content_size - region->start - region->size
            );
            content_size -= region->size;
            regions.erase(region);
        }

        const std::list<Region>::iterator update(const std::list<Region>::iterator region, T* data, size_t size){
            if(region.size == size){
                std::memcpy(internal_data + region->start, data, size * sizeof(T));
                return region;
            }
            remove(region);
            return append(data,size);
        }
};