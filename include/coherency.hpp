#pragma once

#include <structure/allocator.hpp>
#include <cstring>

/**
 * @brief A list that allows allocation
 * 
 * @tparam T 
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

        /**
         * @brief If source is nullptr only allocates
         * 
         * @param source source data to copy
         * @param size size for the allocation
         * @return size_t an index of the allocation
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

/**
 * @brief A list that keeps all its contents tighly packed.
 * 
 * Uses a system of virtual regions to keep track of contents iterators to which remains valid even on removal and addition
 * 
 * @tparam T 
 */
template <typename T>
class CoherentList{
    public:
        struct Region{
            size_t start = 0;
            size_t size  = 0;
        };

        using RegionIterator = typename std::list<CoherentList<T>::Region>::iterator;
    private:
        std::list<Region> regions = {};
        std::vector<T> internal_data = {};

    public:
        CoherentList(){}
        /**
         * @brief Appends data to the end
         * 
         * @param data data to copy
         * @param size count of the elements
         * @return const RegionIterator 
         */
        const RegionIterator append(const T* data, const size_t size){
            RegionIterator region_iter = 
                regions.insert(regions.end(), {internal_data.size(), size});


            internal_data.insert(internal_data.end(), data, data + size);
            
            return region_iter;
        }
        /**
         * @brief Removes a region
         * 
         * @param region 
         */
        void remove(const RegionIterator region){
            size_t total_to_move = (internal_data.size() - region->start - region->size);
            if(total_to_move > 0){
                std::memmove(
                    internal_data.data() + region->start,
                    internal_data.data() + region->start + region->size,
                    total_to_move * sizeof(T)
                );
                internal_data.resize(internal_data.size() - region->size);
            }

            for (auto it = region; it != regions.end(); ++it) it->start -= region->size;
            regions.erase(region);
        }

        /**
         * @brief Updates a region with new data, doesnt reallocate if not neccesarry
         * 
         * @param region 
         * @param data 
         * @param size 
         * @return const RegionIterator 
         */
        const RegionIterator update(const RegionIterator region, const T* data, const size_t size){
            if(region->size == size){
                std::memcpy(internal_data.data() + region->start, data, size * sizeof(T));
                return region;
            }
            remove(region);
            return append(data,size);
        }

        T* data() {return internal_data.data(); };
        size_t size() {return internal_data.size(); };
};