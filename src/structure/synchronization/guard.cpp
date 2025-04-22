#include <shared_mutex>
#include <structure/synchronization/guard.hpp>

using namespace Sync;

Lock<std::unique_lock<std::shared_mutex>> Guard::Unique() {
    if (!unique_locked)
        return Lock<std::unique_lock<std::shared_mutex>>(mutex, &unique_locked);

    return Lock<std::unique_lock<std::shared_mutex>>();
}
Lock<std::shared_lock<std::shared_mutex>> Guard::Shared() {
    std::shared_lock<std::shared_mutex> lock;

    if (!shared_locked)
        return Lock<std::shared_lock<std::shared_mutex>>(mutex, &shared_locked);

    return Lock<std::shared_lock<std::shared_mutex>>();
}
