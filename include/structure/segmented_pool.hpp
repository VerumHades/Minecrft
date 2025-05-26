#pragma once

#include <vector>
#include <queue>
#include <cpptrace/cpptrace.hpp>
#include <mutex>

/**
 * @brief A pool that allocates segments of elements of set size instead of one element at a time
 *
 * @tparam T
 */
template <typename T> class SegmentedPool {
  public:
    /**
     * @brief Internal class to represent a segment that cleans itself up
     *
     */
    class Segment {
      private:
        SegmentedPool<T>* pool = nullptr;

        size_t start;
        size_t size;

        Segment(SegmentedPool<T>* pool, size_t start, size_t size) : pool(pool), start(start), size(size) {}

        friend class SegmentedPool<T>;

      public:
        Segment(const Segment&)            = delete;
        Segment& operator=(const Segment&) = delete;

        Segment(Segment&& other) noexcept : pool(other.pool), start(other.start), size(other.size) {
            other.pool = nullptr; // Prevent freeing in destructor
            other.size = 0;
        }

        // Move assignment
        Segment& operator=(Segment&& other) noexcept {
            if (this != &other) {
                // Free current segment if valid
                if (pool) {
                    pool->Free(start);
                }

                // Transfer ownership
                pool  = other.pool;
                start = other.start;
                size  = other.size;

                // Nullify source
                other.pool = nullptr;
                other.size = 0;
            }
            return *this;
        }

        ~Segment() {
            if (pool)
                pool->Free(start);
        }

        T& operator[](size_t index) {
            if (index < 0 || index > size)
                throw cpptrace::out_of_range("Segment index out of range." + std::to_string(index) + " > " + std::to_string(size));
            return pool->vec[start + index];
        }

        const T& operator[](size_t index) const {
            if (index < 0 || index > size)
                throw cpptrace::out_of_range("Segment index out of range." + std::to_string(index) + " > " + std::to_string(size));
            return pool->vec[start + index];
        }

        size_t Size() const {
            return size;
        }
    };

  private:
    std::vector<T> vec;
    std::queue<size_t> free;
    size_t count      = 0;
    size_t block_size = 1;

    std::mutex mutex;

    void ExtendIfNeeded() {
        std::lock_guard lock(mutex);
        while (free.empty()) {
            size_t old_size = vec.size();
            vec.resize(vec.size() * 2);

            if (old_size < block_size)
                return;

            for (size_t i = 0; i < old_size; i += block_size)
                free.push(i + old_size);
        }
    }

    /*
        Marks an index as free
    */
    void Free(size_t index) {
        std::lock_guard lock(mutex);
        count--;
        free.push(index);
    }

  public:
    SegmentedPool(size_t block_size) : block_size(block_size) {
        if (block_size <= 0)
            throw std::logic_error("SegmentedPool segment size cannot be 0.");
        vec = std::vector<T>(block_size);
        free.push(0);
    }

    SegmentedPool() = default;

    // Delete copy constructor and copy assignment
    SegmentedPool(const SegmentedPool&)            = delete;
    SegmentedPool& operator=(const SegmentedPool&) = delete;

    // Delete move constructor and move assignment
    SegmentedPool(SegmentedPool&&)            = delete;
    SegmentedPool& operator=(SegmentedPool&&) = delete;

    ~SegmentedPool() = default;

    /**
     * @brief Returns the next free segment
     *
     * @return Segment
     */
    Segment Next() {
        ExtendIfNeeded();

        std::lock_guard lock(mutex);

        auto index = free.front();
        free.pop();

        count++;
        return Segment{this, index, block_size};
    }

    size_t Count() {
        return count;
    }
};