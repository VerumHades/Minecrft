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
void freeWorld(World* world);

BlockIndex getWorldBlock(World* world,int x, int y, int z);
int setWorldBlock(World* world,int x, int y, int z, BlockIndex index);

Chunk* generateWorldChunk(World* world, int x, int z);
Chunk* getWorldChunk(World* world, int x, int z);
Chunk* getWorldChunkWithMesh(World* world, int x, int z);


int worldCollides(World* world, float x, float y, float z);

#endif