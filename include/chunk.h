#ifndef CHUNK_H
#define CHUNK_H

#include <math.h>
#include <time.h>
#include <perlinNoise.h>

#include <buffer.h>
#include <mesh.h>

#define LAYER_MODE_FILL 1
#define LAYER_MODE_INDIVIDUAL 2

#define OK -1
#define INVALID_COORDINATES -2
#define LAYER_CORRUPTED -3

#define DEFAULT_CHUNK_SIZE 16
#define DEFAULT_CHUNK_AREA DEFAULT_CHUNK_SIZE * DEFAULT_CHUNK_SIZE
#define DEFAULT_CHUNK_HEIGHT 256

#define TEXTURES_TOTAL 4

extern float textureSize;
typedef short BlockIndex;

// A rectangular collider
typedef struct RectangularCollider{
    float x; // Positional offsets
    float y;
    float z;
    float width;
    float height; //Size
    float depth;
} RectangularCollider;

typedef struct BlockType{
    // For rendering
    unsigned char* textures; // top > bottom > left > right > front > back

    unsigned transparent: 1;
    unsigned untextured: 1;
    unsigned repeatTexture: 1;
    struct{
        float r; float g; float b;  
    } color;
    
    // For physics
    RectangularCollider* colliders;
    unsigned int colliderCount;
} BlockType;

typedef struct ChunkLayer{
    unsigned int mode;
    BlockIndex* data;
    BlockIndex block_index;
} ChunkLayer;

typedef struct Chunk{
    unsigned int size_x;
    unsigned int size_y;
    unsigned int size_z;

    int worldX;
    int worldZ;
    struct World* world;

    struct ChunkLayer* layers;

    unsigned meshGenerating: 1;
    unsigned meshGenerated: 1;

    unsigned buffersLoaded: 1;
    unsigned buffersAsigned: 1;

    unsigned isDrawn: 1;
    
    Mesh* solidMesh;
    Mesh* transparentMesh;

    GLBuffer solidBuffer;
    GLBuffer solidBackBuffer;

    GLBuffer transparentBuffer;
    GLBuffer transparentBackBuffer;
} Chunk; 

typedef struct FaceDefinition{
    int offsetX; int offsetY; int offsetZ;
    int* vertexIndexes;
    int textureIndex;
} FaceDefinition;

typedef struct World World;
extern BlockType predefinedBlocks[];

BlockIndex getChunkBlock(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z);
int setChunkBlock(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z, BlockIndex  value);

Chunk* generatePlainChunk(BlockIndex  top, BlockIndex  rest);
Chunk* generatePerlinChunk(World* world, int chunkX, int chunkZ);

void generateMeshForChunk(Mesh* solid, Mesh* transparent, Chunk* chunk);

#include <world.h>

#endif