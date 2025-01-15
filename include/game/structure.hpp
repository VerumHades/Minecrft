#pragma once

#include <blockarray.hpp>
#include <vector>
#include <vec_hash.hpp>
#include <unordered_set>

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

        using PositionSet = std::unordered_set<glm::ivec3, IVec3Hash, IVec3Equal>;

        /*
            Places structure and returns all chunk positions that need updating
        */
        PositionSet place(const glm::ivec3& position, World& world);   
        
        static Structure capture(const glm::ivec3& position, const glm::ivec3& size, const World& world);
        glm::ivec3 getSize(){return {width,height,depth};}

        ByteArray serialize();
        static Structure deserialize(ByteArray& source_array);
};

#include <game/world/world.hpp>