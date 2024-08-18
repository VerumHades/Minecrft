#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <chunk.hpp>

typedef struct Biome{
    int topBlock;
    int secondaryTopBlock;
    int undergroundBlock;

    float temperature_lower;
    float temperature_upper;

    void (*generateTree)(Chunk* chunk, int x, int y, int z);
} Biome;

Chunk* generateTerrainChunk(World* world, int x, int z);

#endif