#pragma once

#include "vec_hash.hpp"
#include <game/chunk.hpp>
#include <game/save_structure.hpp>

#include <structure/record_store.hpp>
#include <structure/serialization/octree_serializer.hpp>
#include <structure/octree.hpp>

#include <fstream>
#include <string>
#include <shared_mutex>
#include <random>
#include <limits>
#include <iostream>

class WorldStream: public FileStream{
    private:
        constexpr static int segment_size = 4;
        std::shared_mutex mutex;

        struct Header{
            char name[256];
            int seed = 0;
        };

        glm::ivec3 GetSegmentPositionFor(const glm::ivec3& position);

        using Segment = Octree<Chunk>;

        struct SegmentPack {
            Segment segment;

            std::atomic<int> in_use_by = 0;
            std::atomic<bool> marked_for_deletion = false;

            std::shared_mutex segment_mutex;
        };

        Cache<glm::ivec3, std::unique_ptr<SegmentPack>, IVec3Hash, IVec3Equal> segment_cache{40};

        std::mutex marking_mutex;
        std::unordered_map<glm::ivec3, std::unique_ptr<SegmentPack>, IVec3Hash, IVec3Equal> marked_for_deletion;


        using SegmentRecordStore = RecordStore<glm::ivec3, Header, IVec3Hash, IVec2Equal>;
        SegmentRecordStore record_store;

        // Thread safe, Returns whether the segment could be loaded
        bool LoadSegment(const glm::ivec3& position);
        // Thread safe
        void LoadSegmentToCache(const glm::ivec3& position, const std::unique_ptr<SegmentPack>& segment);
        // Thread safe
        void SaveSegment(const glm::ivec3& position, const SegmentPack& segment);
        // Thread safe
        void CreateSegment(const glm::ivec3& position);

        // Thread safe
        SegmentPack* GetSegment(const glm::ivec3& position, bool set_in_use = false);

        // Thread safe
        void StopUsingSegment(SegmentPack* segment, const glm::ivec3& position);

    public:
        WorldStream(const std::string& path);
        ~WorldStream();
        //bool save(Chunk& chunk);
        //void load(Chunk* chunk);
        bool HasChunkAt(const glm::ivec3& position);

        int GetSeed() const;
        void SetSeed(int value);

        std::string GetName() const;
        void SetName(const std::string& name);

        bool Save(const std::unique_ptr<Chunk>& chunk);
        std::unique_ptr<Chunk> Load(const glm::ivec3& position);
};
