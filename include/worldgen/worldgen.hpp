#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <chunk.hpp>
#include <optional>

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

Chunk generateTerrainChunk(World& world, int x, int z);

#endif