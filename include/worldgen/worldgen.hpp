#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <game/chunk.hpp>
#include <optional>
#include <random>

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

void generateTerrainChunk(Chunk& chunk, int chunkX, int chunkY, int chunkZ);

#endif