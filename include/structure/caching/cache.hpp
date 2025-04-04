#pragma once

#include <functional>
#include <unordered_map>
#include <optional>

#include <structure/caching/eviction_policies.hpp>

template <typename Key, typename T, typename EvictionPolicy = CacheEvictionPolicies::LeastRecentlyUsed<Key>,
          typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>>
class Cache {
  private:
    std::unordered_map<Key, T, Hash, Equal> cached_values{};

    size_t max_cached_elements = 500;
    EvictionPolicy eviction_policy;

  public:
    Cache(size_t max_cached = 500): max_cached_elements(max_cached) {}

    T* Get(const Key& key) {
        eviction_policy.KeyRequested(key);

        if (cached_values.contains(key))
            return &cached_values.at(key);

        if (cached_values.size() > max_cached_elements){
            Key evicted_key = eviction_policy.Evict();
            cached_values.erase(evicted_key);
        }

        auto opt = load_function(key);
        if(!opt) return nullptr;
        cached_values.insert_or_assign(key, std::move(opt.value()));
        return &cached_values.at(key);
    }

    /*
        If a cached value was evicted returns it with its key, otherwise returns std::nullopt
    */
    std::optional<std::pair<Key, T>> Load(const Key& key, const T& t){
        std::optional<std::pair<Key, T>> result = std::nullopt;

        if (cached_values.size() > max_cached_elements){
            auto node = cached_values.extract(eviction_policy.Evict());

            if (node) {
                result = std::make_pair(std::move(node.key()), std::move(node.mapped()));
            }
        }

        eviction_policy.KeyRequested(key);
        cached_values.insert_or_assign(key, std::move(t));
        return std::move(result);
    }

    void Clear(const std::function<void(Key, T)> evict){
        for(auto& [key, value]: cached_values)
            evict(key,value);

        cached_values.clear();
    }
};
