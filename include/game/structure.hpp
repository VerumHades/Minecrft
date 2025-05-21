#pragma once

#include <structure/serialization/serializer.hpp>

#include <blockarray.hpp>
#include <vector>
#include <vec_hash.hpp>
#include <unordered_set>
#include <shared_mutex>
#include <mutex>


class Terrain;

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

        friend class Serializer;

    public: 
        Structure(uint width, uint height, uint depth): width(width), height(height), depth(depth) {}

        void setBlock(glm::ivec3 position, const Block& block);
        Block* getBlock(glm::ivec3 position);

        using PositionSet = std::unordered_set<glm::ivec3, IVec3Hash, IVec3Equal>;

        /*
            Places structure and returns all chunk positions that need updating
        */
        PositionSet place(const glm::ivec3& position, Terrain& world);   
        
        static Structure capture(const glm::ivec3& position, const glm::uvec3& size, const Terrain& world);
        glm::ivec3 getSize(){return {width,height,depth};}
};

#include <game/world/terrain.hpp>