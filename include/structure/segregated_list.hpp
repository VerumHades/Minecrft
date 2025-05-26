#pragma once

#include <vector>
#include <unordered_map>

/**
 * @brief A list that segregates elements by a key
 * 
 * @tparam K 
 * @tparam T 
 */
template <typename K, typename T>
class SegregatedList{
    private:
        std::unordered_map<K, std::vector<T>> groups;
        const std::vector<T> empty{};
        
    public:
        /**
         * @brief Add element
         * 
         * @param key 
         * @param values 
         */
        void Push(const K& key, const std::vector<T>& values){
            if(!groups.contains(key)) groups[key] = {};

            auto& list = groups.at(key);
            list.insert(list.end(), values.begin(), values.end());
        }

        /**
         * @brief Check if key exists
         * 
         * @param key 
         * @return true 
         * @return false 
         */
        bool HasKey(const K& key) const {
            return groups.contains(key);
        }
        
        // Preallocates
        void Reserve(const K& key, size_t size){
            if(!groups.contains(key)) groups[key] = {};
            
            groups.at(key).reserve(size);
        }

        /**
         * @brief Shrink all vectors to size
         * 
         */
        void Shrink(){
            for(auto& [key,group]: groups) group.shrink_to_fit();
        }

        /**
         * @brief Get a vector under a key
         * 
         * @param key 
         * @return const std::vector<T>& 
         */
        const std::vector<T>& Get(const K& key) const {
            if(!groups.contains(key)) return empty;
            return groups.at(key);
        }

        /**
         * @brief Get the All vectors
         * 
         * @return const std::unordered_map<K, std::vector<T>>& 
         */
        const std::unordered_map<K, std::vector<T>>& GetAll() const {
            return groups;
        }
};