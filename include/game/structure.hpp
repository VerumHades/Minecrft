#pragma once

#include <blockarray.hpp>
#include <vector>

class World;

class Structure{
    private:
        uint width = 0;
        uint height = 0;
        uint depth = 0;

        std::vector<SparseBlockArray> block_arrays{};

        /*
            It it doesnt exist returns nullptr
        */
        std::tuple<SparseBlockArray*, glm::ivec3> getBlockArrayForPosition(glm::ivec3 position);

    public: 
        Structure(uint width, uint height, uint depth): width(width), height(height), depth(depth) {
            block_arrays = std::vector<SparseBlockArray>(
                ceil((float)width  / 64.0f) * 
                ceil((float)height / 64.0f) * 
                ceil((float)depth  / 64.0f)
            );
        }

        void setBlock(glm::ivec3 position, const Block& block);
        Block* getBlock(glm::ivec3 position);

        void place(const glm::ivec3& position, World& world);   
        
        static Structure capture(const glm::ivec3& position, const glm::ivec3& size, const World& world);
};

#include <game/world/world.hpp>