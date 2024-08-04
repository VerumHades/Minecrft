#ifndef WORLD_H
#define WORLD_H

#include <chunk.h>
#include <hashmap.h>
#include <threads.h>

#define FORMATED_STRING(output, source, ...) \
        char* output = calloc(1,snprintf(NULL, 0,source, __VA_ARGS__)+1); \
        sprintf(output, source, __VA_ARGS__);


typedef struct World{
    HashMap* chunks;
} World;

World* newWorld();

BlockIndex getWorldBlock(World* world,int x, int y, int z);
Chunk* generateWorldChunk(World* world, int x, int z);
Chunk* getWorldChunk(World* world, int x, int z);
Chunk* getWorldChunkWithMesh(World* world, int x, int z);

void freeWorld(World* world);

#endif