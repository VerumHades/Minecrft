#pragma once

#include <list>
#include <unordered_map>

namespace CacheEvictionPolicies{
    /**
     * @brief A least recently used eviction policy for caching
     * 
     * @tparam Key 
     * @tparam Hash 
     * @tparam Equal 
     */
    template <typename Key, typename Hash, typename Equal>
    class LeastRecentlyUsed {
    private:
        std::list<Key> access_order;
        std::unordered_map<Key, typename std::list<Key>::iterator, Hash, Equal> key_positions;

    public:
        void KeyRequested(const Key& key) {
            auto it = key_positions.find(key);
            if (it != key_positions.end()) {
                access_order.erase(it->second);
            }

            access_order.push_front(key);
            key_positions[key] = access_order.begin();
        }

        Key Evict() {
            Key lru_key = access_order.back();
            access_order.pop_back();
            key_positions.erase(lru_key);
            return lru_key;
        }
    };
}
