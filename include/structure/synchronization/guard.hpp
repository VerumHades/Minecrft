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

/**
 * @brief A flexible class that allows locking for larger scopes, thus allowing performance gains
 * 
 */
class Guard {
  private:
    std::shared_mutex mutex;
    std::atomic<bool> unique_locked = false;
    std::atomic<bool> shared_locked = false;

  public:
  /**
   * @brief Aquire a unique lock
   * 
   * @return Lock<std::unique_lock<std::shared_mutex>> 
   */
    Lock<std::unique_lock<std::shared_mutex>> Unique();
    /**
   * @brief Aquire a shared lock
   * 
   * @return Lock<std::unique_lock<std::shared_mutex>> 
   */
    Lock<std::shared_lock<std::shared_mutex>> Shared();
};
} // namespace Sync
