#pragma once

#include <structure/definitions.hpp>
#include <vector>

template <typename Key>
class KeyedStorage{
    public:
        /*
            Save some data under a key
        */
        virtual void Save(const Key& key, size_t size = 0, const byte* data = nullptr) = 0;

        /*
            Load data stored under a key into a vector (overwrites),
            returns true if data was read,
            returns false if it wasnt
        */
        virtual bool Get(const Key& key, std::vector<byte>& output) = 0;
};
