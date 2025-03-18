#pragma once

#include <vector>
#include <unordered_map>

template <typename K, typename T>
class SegregatedList{
    private:
        std::unordered_map<K, std::vector<T>> groups;
    
    public:
        void Push(const K& key, const std::vector<T>& values){
            if(!groups.contains(key)) groups[key] = {};

            auto& list = groups.at(key);
            list.insert(list.end(), values.begin(), values.end());
        }

        bool HasKey(const K& key) const {
            return groups.contains(key);
        }
        
        // Preallocates
        void Reserve(const K& key, size_t size){
            if(!groups.contains(key)) groups[key] = {};
            
            groups.at(key).reserve(size);
        }

        // Shrinks vectors to size 
        void Shrink(){
            for(auto& [key,group]: groups) group.shrink_to_fit();
        }

        const std::vector<T>& Get(const K& key) const {
            return groups.at(key);
        }

        const std::unordered_map<K, std::vector<T>>& GetAll() const {
            return groups;
        }
};