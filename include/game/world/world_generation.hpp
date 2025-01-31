#pragma once

#include <optional>
#include <random>
#include <game/blocks.hpp>

#include <memory>
#include <rendering/opengl/shaders.hpp>
#include <rendering/opengl/texture.hpp>

#include <parsing/source_template.hpp>

#include <FastNoiseLite.h> 

class Chunk;
class Terrain;

class WorldGenerator{
    private:
        FastNoiseLite noise;
        int seed;
        
    public:
        WorldGenerator();

        void generateTerrainChunk(Chunk* chunk, glm::ivec3 position);

        void setSeed(int seed) {
            noise.SetSeed(seed);
            this->seed = seed;
        }
};

#include <game/chunk.hpp>
#include <game/world/terrain.hpp>