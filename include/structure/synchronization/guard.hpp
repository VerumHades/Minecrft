#pragma once

#include <shared_mutex>
#include <mutex>
#include <atomic>

namespace Sync {

class Guard;

template <typename T> class Lock {
  private:
    T lock;
    std::atomic<bool>* flag;

    Lock(std::shared_mutex& mutex, std::atomic<bool>* flag) : lock(mutex), flag(flag) {
        *flag = true;
    }
    Lock() : lock(), flag(nullptr) {}

    friend class Guard;

  public:
    ~Lock() {
        if (flag)
            *flag = false;
    }
};

class Guard {
  private:
    std::shared_mutex mutex;
    std::atomic<bool> unique_locked = false;
    std::atomic<bool> shared_locked = false;

  public:
    Lock<std::unique_lock<std::shared_mutex>> Unique();
    Lock<std::shared_lock<std::shared_mutex>> Shared();
};
} // namespace Sync
