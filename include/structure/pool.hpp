#pragma once

#include <vector>
#include <functional>

template <typename T>
class Pool{
    private:
        std::vector<T> elements;
        std::queue<size_t> next_draw_index;
        std::function<void(T*)> clear_cell;

        friend class PoolIndex;

    public:
        Pool(const std::function<void(T*)>& clear_cell): clear_cell(clear_cell) {}

        Pool( const Pool& ) = delete; // non construction-copyable
        Pool& operator=( const Pool& ) = delete; // non copyable

        class PoolIndex{
            private:
                size_t index;
                Pool<T>* parent;

            public:
                T* operator->() {
                    return &target;
                }
        };

        std::shared_ptr<T> Add(const auto& element){
            if(next_draw_index.empty()){
                size_t size = elements.size();
                element.resize(size * 2);
                for(int i = 0; i < size;i++) next_draw_index.push(size + i);
            }

            size_t index = next_draw_index.front();
            next_draw_index.pop();

            elements[index] = element;
            
            return std::make_shared<T>(&(*iterator), [this](T*){
                next_draw_index.push(index);  
                if(clear_cell) clear_cell(&elements[index]);
            });
        }

        T* Data(){return element.data();}
};