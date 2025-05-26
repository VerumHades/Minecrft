#pragma once

#include <unordered_map>
#include <mutex>

template <typename T> class ThreadLocal {
  private:
    std::mutex mutex;
    std::unordered_map<std::thread::id, std::unique_ptr<T>> instance_map;

  public:
    T& Get() {
        std::thread::id this_id = std::this_thread::get_id();
        std::lock_guard lock(mutex);

        if (instance_map.contains(this_id))
            return *instance_map.at(this_id).get();

        auto [iter, inserted] = instance_map.emplace(this_id, std::make_unique<T>());
        if (!inserted)
            throw std::logic_error("Failed to emplace into threadlocal map.");
        return *iter->second.get();
    }
    void Drop() {
        std::thread::id this_id = std::this_thread::get_id();
        std::lock_guard lock(mutex);
        if (instance_map.contains(this_id))
            instance_map.erase(this_id);
    }
};
