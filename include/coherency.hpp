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
    A list that keeps all its contents tighly packed, 
    uses a system of virtual regions to keep track of contents iterators to which remains valid even on removal and addition
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

        const RegionIterator append(const T* data, const size_t size){
            RegionIterator region_iter = 
                regions.insert(regions.end(), {content_size, size});

            if(internal_size < content_size + size){
                T* old_data = internal_data;
                
                while(internal_size < content_size + size) internal_size *= 2;
                internal_data = new T[internal_size];

                std::memcpy(internal_data, old_data, content_size * sizeof(T));

                delete old_data;
            }

            std::memcpy(internal_data + content_size, data, size * sizeof(T));
            content_size += size;
            
            return region_iter;
        }

        void remove(const RegionIterator region){
            size_t total_to_move = (content_size - region->start - region->size);
            if(total_to_move > 0){
                std::memmove(
                    internal_data + region->start,
                    internal_data + region->start + region->size,
                    total_to_move * sizeof(T)
                );
                content_size -= region->size;
            }

            for (auto it = region; it != regions.end(); ++it) it->start -= region->size;
            regions.erase(region);
        }

        const RegionIterator update(const RegionIterator region, const T* data, const size_t size){
            if(region->size == size){
                std::memcpy(internal_data + region->start, data, size * sizeof(T));
                return region;
            }
            remove(region);
            return append(data,size);
        }

        T* data() {return internal_data; };
        size_t size() {return content_size; };
};

/*
    Similar to the vector just without any additional checks, be careful when using it
*/
template <typename T>
class List{
    private:
        T* _data  = nullptr;
        size_t _size = 0;
        size_t count = 0;

    public:
        List(){
            _data = new T[1];
            _size = 1;
        }
        ~List(){
            delete _data;
        }
        void push(T data){
            if(count + 1 >= _size) resize(_size * 2);

            _data[count++] = data;
        }
        void push(T* data, size_t size){
            if(count + size >= _size){
                size_t new_size = _size * 2;
                while(count + size >= new_size) new_size *= 2;
                resize(_size * 2);
            }

            std::memcpy(_data + count, data, size * sizeof(T));
            count += size;
        }
        /*
            Resizes the actual size doesnt interact with count
        */
        void resize(size_t size){
            if(size == 0) throw std::logic_error("Invalid list size: 0");
            size_t copy_size = std::min(count, size);

            if(size < count) count = size;

            T* old_data = _data;
            _data = new T[size];
            
            std::memcpy(_data, old_data, copy_size * sizeof(T));
            _size = size;

            delete old_data;
        }

        // Clears the list, doesnt delete anything nor does it resize tho
        void clear(){
            count = 0;
        }
        
        T& operator[](std::size_t index) {
            return data[index];
        }

        const T& operator[](std::size_t index) const {
            return data[index];
        }

        size_t size(){
            return count;
        }

        T* data(){
            return _data;
        }
};