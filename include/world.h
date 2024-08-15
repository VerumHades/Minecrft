#ifndef WORLD_H
#define WORLD_H

#include <chunk.h>
#include <structures/posmap.h>
#include <time.h>

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


typedef struct StoredChunk{
    int worldX;
    int worldZ;
    Block blocks[DEFAULT_CHUNK_SIZE][DEFAULT_CHUNK_HEIGHT][DEFAULT_CHUNK_SIZE];
} StoredChunk;

typedef struct StoredWorldMetadata{
    char head[6];
    int storedChunksTotal;
} StoredWorldMetadata;

typedef struct World{
    char* storageFilename;
    FILE* file;
    StoredWorldMetadata metadata;

    #ifdef _WIN32

    #else
    mtx_t threadlock;
    #endif

    PositionMap* chunks;
    PositionMap* storedIndices;
} World;

typedef struct RaycastResult{
    Block* hitBlock;
    unsigned hit: 1;
    int x;
    int y;
    int z;

    float lastX;
    float lastY;
    float lastZ;
} RaycastResult;

typedef struct CollisionCheckResult{
    Block* collidedBlock;
    unsigned collision: 1;
    int x;
    int y;
    int z;
} CollisionCheckResult;

World* newWorld(char* storageFilename);
void freeWorld(World* world);
void updateWorldStorageRegistry(World* world);
void saveWorld(World* world);
int writeWorldMetadata(World* world);

Block* getWorldBlock(World* world,int x, int y, int z);
int setWorldBlock(World* world,int x, int y, int z, Block index);

Chunk* generateWorldChunk(World* world, int x, int z);
Chunk* getWorldChunk(World* world, int x, int z);
Chunk* getWorldChunkWithMesh(World* world, int x, int z, ShaderProgram* program);
Chunk* getChunkFromBlockPosition(World* world, int x, int z);
void regenerateChunkMesh(Chunk* chunk);

CollisionCheckResult checkForPointCollision(World* world, float x, float y, float z, int includeRectangularColliderLess);
CollisionCheckResult checkForRectangularCollision(World* world, float x, float y, float z, RectangularCollider* collider);

RaycastResult raycast(World* world, float fromX, float fromY, float fromZ, float dirX, float dirY, float dirZ, float maxDistance);
RaycastResult raycastFromAngles(World* world, float fromX, float fromY, float fromZ, int angleX, int angleY, float maxDistance);

#endif