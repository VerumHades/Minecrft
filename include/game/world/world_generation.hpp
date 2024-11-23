#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <optional>
#include <random>
#include <game/blocks.hpp>

#include <memory>
#include <rendering/shaders.hpp>

#include <FastNoiseLite.h> 

class Chunk;

struct Biome {
    BlockType topBlock;
    BlockType secondaryTopBlock;
    BlockType undergroundBlock;

    float temperatureLower;
    float temperatureUpper;

    using GenerateTreeFunc = void (*)(Chunk&, int, int, int);

    Biome(BlockType top, BlockType secondaryTop, BlockType underground,
          float tempLower, float tempUpper)
        : topBlock(top), secondaryTopBlock(secondaryTop), undergroundBlock(underground),
          temperatureLower(tempLower), temperatureUpper(tempUpper){}
};

class WorldGenerator{
    private:
        FastNoiseLite noise;
        int seed;

        uint worldPositionUniformID;

        ShaderProgram computeProgram;
        std::unique_ptr<GLPersistentBuffer<uint>> computeBuffer;

    public:
        WorldGenerator(int seed);
        WorldGenerator(){seed = 1948;}

        void generateTerrainChunk(Chunk* chunk, int chunkX, int chunkY, int chunkZ, size_t size);
        
        /*
            A gpu accelerated generation function
        */
        void generateTerrainChunkAccelerated(Chunk* chunk, glm::ivec3 chunkPosition);
};

#include <game/chunk.hpp>

#endif