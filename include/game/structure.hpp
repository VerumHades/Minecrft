#pragma once

#include <blockarray.hpp>
#include <vector>
#include <vec_hash.hpp>

class World;

class Structure{
    private:
        uint width = 0;
        uint height = 0;
        uint depth = 0;

        std::unordered_map<glm::ivec3, SparseBlockArray, IVec3Hash, IVec3Equal> block_arrays{};

        /*
            It it doesnt exist returns nullptr
        */
        std::tuple<SparseBlockArray*, glm::ivec3> getBlockArrayForPosition(glm::ivec3 position);

    public: 
        Structure(uint width, uint height, uint depth): width(width), height(height), depth(depth) {}

        void setBlock(glm::ivec3 position, const Block& block);
        Block* getBlock(glm::ivec3 position);

        void place(const glm::ivec3& position, World& world);   
        
        static Structure capture(const glm::ivec3& position, const glm::ivec3& size, const World& world);

        ByteArray serialize();
        static Structure deserialize(ByteArray& source_array);
};

#include <game/world/world.hpp>