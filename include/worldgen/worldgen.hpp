#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <chunk.hpp>
#include <optional>

struct Biome {
     BlockTypeEnum topBlock;
    BlockTypeEnum secondaryTopBlock;
    BlockTypeEnum undergroundBlock;

    float temperatureLower;
    float temperatureUpper;

    using GenerateTreeFunc = void (*)(Chunk*, int, int, int);

    Biome(BlockTypeEnum top, BlockTypeEnum secondaryTop, BlockTypeEnum underground,
          float tempLower, float tempUpper,
          GenerateTreeFunc treeGen)
        : topBlock(top), secondaryTopBlock(secondaryTop), undergroundBlock(underground),
          temperatureLower(tempLower), temperatureUpper(tempUpper), generateTree(treeGen) {}

    GenerateTreeFunc generateTree;
};

Chunk* generateTerrainChunk(World* world, int x, int z);

#endif