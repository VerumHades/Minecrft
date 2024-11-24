#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <optional>
#include <random>
#include <game/blocks.hpp>

#include <memory>
#include <rendering/shaders.hpp>

#include <FastNoiseLite.h> 

class Chunk;

class WorldGenerator{
    private:
        FastNoiseLite noise;
        int seed;

        uint worldPositionUniformID;

        ShaderProgram computeProgram;
        std::unique_ptr<GLPersistentBuffer<uint>> computeBuffer;
        BlockRegistry& blockRegistry;

    public:
        WorldGenerator(BlockRegistry&  blockRegistry);

        void generateTerrainChunk(Chunk* chunk, int chunkX, int chunkY, int chunkZ, size_t size);
        
        /*
            A gpu accelerated generation function
        */
        void generateTerrainChunkAccelerated(Chunk* chunk, glm::ivec3 chunkPosition);

        void setSeed(int seed) {
            noise.SetSeed(seed);
            this->seed = seed;
        }
};

#include <game/chunk.hpp>

#endif