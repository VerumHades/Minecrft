#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <optional>
#include <random>
#include <game/blocks.hpp>
#include <memory>

#include <FastNoiseLite.h> 

class Chunk;

struct Biome {
    BlockTypes topBlock;
    BlockTypes secondaryTopBlock;
    BlockTypes undergroundBlock;

    float temperatureLower;
    float temperatureUpper;

    using GenerateTreeFunc = void (*)(Chunk&, int, int, int);

    Biome(BlockTypes top, BlockTypes secondaryTop, BlockTypes underground,
          float tempLower, float tempUpper,
          GenerateTreeFunc treeGen)
        : topBlock(top), secondaryTopBlock(secondaryTop), undergroundBlock(underground),
          temperatureLower(tempLower), temperatureUpper(tempUpper), generateTree(treeGen) {}

    GenerateTreeFunc generateTree;
};

class WorldGenerator{
    private:
        FastNoiseLite noise;
        int seed;
    public:
        WorldGenerator(int seed);
        WorldGenerator(){seed = 1948;}
        void generateTerrainChunk(Chunk& chunk, int chunkX, int chunkY, int chunkZ);
};

#include <game/chunk.hpp>

#endif