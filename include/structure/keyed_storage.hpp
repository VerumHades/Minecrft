#pragma once

#include <structure/definitions.hpp>
#include <vector>

/**
 * @brief A generic definition of a keyed storage
 * 
 * @tparam Key 
 */
template <typename Key>
class KeyedStorage{
    public:
        /**
         * @brief Save some data under a key
         * 
         * @param key 
         * @param size 
         * @param data 
         * @return Save 
         */
        virtual void Save(const Key& key, size_t size = 0, const byte* data = nullptr) = 0;

        /**
         * @brief Load data stored under a key into a vector (overwrites),
         * 
         * @param key 
         * @param output 
         * @return true if data was read
         * @return false if it wasnt
         */
        virtual bool Get(const Key& key, std::vector<byte>& output) = 0;
};
