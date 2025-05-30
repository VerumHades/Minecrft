#pragma once

#include "structure/keyed_storage.hpp"
#include "vec_hash.hpp"
#include <game/chunk.hpp>
#include <game/save_structure.hpp>

#include <structure/serialization/octree_serializer.hpp>
#include <structure/caching/cache.hpp>
#include <structure/octree.hpp>

#include <fstream>
#include <string>
#include <shared_mutex>
#include <random>
#include <limits>
#include <iostream>

/**
 * @brief A class responsible for storing chunks into a file
 * 
 * Thread safe
 */
class WorldStream {
    private:
        constexpr static int segment_size = 4;
        std::shared_mutex record_mutex;

        glm::ivec3 GetSegmentPositionFor(const glm::ivec3& position);

        using Segment = Octree<Chunk>;

        struct SegmentPack {
            Segment segment{};
            std::shared_mutex segment_mutex;
        };

        using SegmentRecordStore = std::shared_ptr<KeyedStorage<glm::ivec3>>;
        SegmentRecordStore record_store;

        Cache<glm::ivec3, std::shared_ptr<SegmentPack>, IVec3Hash, IVec3Equal> segment_cache{40};

        // Thread safe, Returns whether the segment could be loaded
        bool LoadSegment(const glm::ivec3& position);
        // Thread safe
        void LoadSegmentToCache(const glm::ivec3& position, std::shared_ptr<SegmentPack> segment);
        // Thread safe
        void SaveSegment(const glm::ivec3& position, SegmentPack* segment);
        // Thread safe
        void CreateSegment(const glm::ivec3& position);

        // Thread safe
        std::shared_ptr<SegmentPack> GetSegment(const glm::ivec3& position, bool set_in_use = false);

        void Flush();

    public:
        WorldStream(const std::shared_ptr<KeyedStorage<glm::ivec3>>& storage);
       // ~WorldStream();
        
        /**
         * @brief Check whether there is a chunk stored for the given position
         * 
         * @param position 
         * @return true 
         * @return false 
         */
        bool HasChunkAt(const glm::ivec3& position);

        /**
         * @brief Stores a chunk, uses its world position as its position
         * 
         * @param chunk 
         * @return true 
         * @return false 
         */
        bool Save(std::unique_ptr<Chunk> chunk);

        /**
         * @brief Loads a chunk for a given position
         * 
         * @param position 
         * @return std::unique_ptr<Chunk> nullptr if it doesnt exist
         */
        std::unique_ptr<Chunk> Load(const glm::ivec3& position);
};
