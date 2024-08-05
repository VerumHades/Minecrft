#ifndef WORLD_H
#define WORLD_H

#include <chunk.h>
#include <hashmap.h>

#ifdef _WIN32

#include <windows.h>
typedef HANDLE thread_t;
#define THREAD_SUCCESS 0
#define THREAD_ERROR INVALID_HANDLE_VALUE

#else

#include <threads.h>
typedef thrd_t thread_t;
#define THREAD_SUCCESS thrd_success
#define THREAD_ERROR NULL

#endif

#define FORMATED_STRING(output, source, ...) \
        char* output = calloc(1,snprintf(NULL, 0,source, __VA_ARGS__)+1); \
        sprintf(output, source, __VA_ARGS__);


typedef struct World{
    HashMap* chunks;
} World;

typedef struct RaycastResult{
    BlockIndex hitBlock;
    unsigned hit: 1;
    int x;
    int y;
    int z;

    float lastX;
    float lastY;
    float lastZ;
} RaycastResult;

typedef struct CollisionCheckResult{
    BlockIndex collidedBlock;
    unsigned collision: 1;
    int x;
    int y;
    int z;
} CollisionCheckResult;

World* newWorld();
void freeWorld(World* world);

BlockIndex getWorldBlock(World* world,int x, int y, int z);
int setWorldBlock(World* world,int x, int y, int z, BlockIndex index);

Chunk* generateWorldChunk(World* world, int x, int z);
Chunk* getWorldChunk(World* world, int x, int z);
Chunk* getWorldChunkWithMesh(World* world, int x, int z);
Chunk* getChunkFromBlockPosition(World* world, int x, int z);
void regenerateChunkMesh(Chunk* chunk);

CollisionCheckResult worldCollides(World* world, float x, float y, float z, int includeAir);
RaycastResult raycast(World* world, float fromX, float fromY, float fromZ, float dirX, float dirY, float dirZ, float maxDistance);
RaycastResult raycastFromAngles(World* world, float fromX, float fromY, float fromZ, int angleX, int angleY, float maxDistance);

#endif