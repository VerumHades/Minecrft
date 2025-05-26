#pragma once


#include <structure/segmented_pool.hpp>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <mutex>

/**
 * @brief A pool that allows allocation of extendable lists to minimize memory fragmentation, it hold several pools that double in size per level
 * 
 * @tparam T 
 */
template <typename T>
class MultilevelPool
{
private:
    std::unordered_map<size_t, std::unique_ptr<SegmentedPool<T>>> pools;

    /**
     * @brief Returns a segment with minimal size, creates it if it doesnt exist
     * 
     * @param size 
     * @return SegmentedPool<T>::Segment 
     */
    typename SegmentedPool<T>::Segment GetSegmentOfMinSize(size_t size)
    {
        std::lock_guard lock(mutex);
        size_t level = ceil(log2(size));

        if (!pools.contains(level))
            pools.emplace(level, std::make_unique<SegmentedPool<T>>(pow(2, level)));

        return pools.at(level)->Next();
    }

    std::mutex mutex;

public:
    MultilevelPool() = default;

    // Delete copy constructor and copy assignment
    MultilevelPool(const MultilevelPool &) = delete;
    MultilevelPool &operator=(const MultilevelPool &) = delete;

    // Delete move constructor and move assignment
    MultilevelPool(MultilevelPool &&) = delete;
    MultilevelPool &operator=(MultilevelPool &&) = delete;

    ~MultilevelPool() = default;

    class List
    {
    private:
        typename SegmentedPool<T>::Segment segment;
        MultilevelPool<T> *pool;
        size_t internal_size = 0;

        List(typename SegmentedPool<T>::Segment segment, MultilevelPool<T> *pool) : segment(std::move(segment)), pool(pool)
        {
        }

        friend class MultilevelPool<T>;

    public:
        // Delete copy constructor and assignment
        List(const List &) = delete;
        List &operator=(const List &) = delete;

        // Move constructor
        List(List &&other) noexcept
            :  segment(std::move(other.segment)), pool(other.pool)
        {
            other.pool = nullptr;
        }

        // Move assignment
        List &operator=(List &&other) noexcept
        {
            if (this != &other)
            {
                pool = other.pool;
                segment = std::move(other.segment);
                other.pool = nullptr;
            }
            return *this;
        }

        void Resize(size_t size)
        {
            auto old_segment = std::move(segment);
            auto old_size = internal_size;
            segment = pool->GetSegmentOfMinSize(size);

            internal_size = std::min(old_size, size);
            for (size_t i = 0; i < internal_size; i++)
                segment[i] = old_segment[i];
        }

        void Push(const T &value)
        {
            if (internal_size >= segment.Size())
                Resize(segment.Size() * 2);

            segment[internal_size++] = value;
        }

        size_t Size() const { return internal_size; }
        const T* Data() const {return &segment[0];};

        T &operator[](size_t index) { return segment[index]; }
        const T &operator[](size_t index) const { return segment[index]; }
    };

    /**
     * @brief Get a list of size
     * 
     * @param size 
     * @return List 
     */
    List Next(size_t size)
    {
        return List{GetSegmentOfMinSize(size), this};
    }

    void Stats(){
        for(auto& [level, pool]: pools){
            std::cout << "Level:" << level << "Level size: " << pow(2,level) << " Count:" << pool->Count() << std::endl;
        }
    }
};
