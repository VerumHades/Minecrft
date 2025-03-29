#pragma once

#include <game/chunk.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <shared_mutex>
#include <random>
#include <game/save_structure.hpp>
#include <limits>

class WorldStream: public FileStream{
    private:
        std::shared_mutex mutex;

        constexpr static size_t NO_RECORD = 0; // If there are no further records
        constexpr static size_t EMTPY = 1; // If there is a record but it signals an empty chunk, only for chunk level records

        struct Header{
            char world_name[256];
            int seed;
            size_t root_index_position;
        };

        struct IndexHeader{
            unsigned char record_level;
            size_t record_indexes[8];
        };

    public:
        WorldStream(const std::string& path);
        ~WorldStream();
        //bool save(Chunk& chunk);
        //void load(Chunk* chunk);
        bool HasChunkAt(glm::vec3 position);

        int GetSeed() const;
        void SetSeed(int value);

        std::string GetName() const;
        void SetName(const std::string& name);

        bool save(Chunk& chunk);
        void load(Chunk* chunk);
};
