#pragma once

#include <vector>
#include <queue>

template <typename T>
class Pool {
    private:
        std::vector<T> vec;
        std::queue<size_t> free;
        size_t count = 0;

        void Extend(){
            size_t old_size = vec.size();
            vec.resize(vec.size() * 2);
            for(size_t i = 0;i < old_size;i++)
                free.push(i + old_size);
        }

    public:
        Pool(){
            vec = std::vector<T>(1);
        }

        /*
            Marks an index as free
        */
        void Free(size_t index){
            count--;
            free.push(index);
        }

        /*
            Returns the next free index, expands the pool if neccesary
        */
        size_t NextIndex(){
            if(free.empty()) Extend();

            auto index = free.front();
            free.pop();

            count++;
            return index;
        }

        T& operator[](size_t index) {
            return vec[index];  // Allows write access
        }

        const T& operator[](size_t index) const {
            return vec[index];  // Allows read-only access
        }

        std::vector<T>& vector() { return vec;}
        size_t Count() {return count;}
};
