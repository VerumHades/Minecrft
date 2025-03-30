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

        struct Header{
            char world_name[256];
            int seed;
            size_t root_index_position;
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

        bool Save(Chunk& chunk);
        void Load(Chunk* chunk);
};
