#pragma once

#include <map>
#include <chrono>

namespace CacheEvictionPolicies{
    template <typename Key>
    class LeastRecentlyUsed{
        private:
            using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

            std::map<TimePoint, Key> timestamps;
        public:
            void KeyRequested(const Key& key){
                timestamps.insert_or_assign(key, std::chrono::high_resolution_clock::now());
            }

            Key Evict(){
                auto [timestamp, key] = timestamps.begin();
                timestamps.erase(timestamp);
                return key;
            }
    };
}
