#pragma once

#include <array>

#include <glm/glm.hpp>
#include <game/chunk.hpp>

#define WORLD_REGION_SIZE 8

class WorldRegion{
    private:
        static const size_t chunks_total = WORLD_REGION_SIZE * WORLD_REGION_SIZE * WORLD_REGION_SIZE;
        std::array<Chunk, WorldRegion::chunks_total> chunks;

        size_t getIndexFromPosition(glm::ivec3 position){
            return position.x + (position.y * WORLD_REGION_SIZE) + (position.z * WORLD_REGION_SIZE * WORLD_REGION_SIZE);
        }
    public:
        /*
            Returns a chunk in the region on coordinates 0-WORLD_REGION_SIZE.

            If positions is out of bounds returns a nullptr;
        */
        Chunk* getChunk(glm::ivec3 position){
            size_t index = getIndexFromPosition(position);
            if(index < 0 || index >= chunks_total) return nullptr;
            return &chunks[index];
        }

        /*
            Generates the whole region using a compute shader all at once.
        */
        void generate(glm::ivec3 world_region_position);
};