#ifndef CHUNK_H
#define CHUNK_H

#include <mesh.h>
#include <math.h>
#include <time.h>
#include <perlinNoise.h>

#include <buffer.h>

#define LAYER_MODE_FILL 1
#define LAYER_MODE_INDIVIDUAL 2

#define OK -1
#define INVALID_COORDINATES -2
#define LAYER_CORRUPTED -3

#define DEFAULT_CHUNK_SIZE 16
#define DEFAULT_CHUNK_AREA DEFAULT_CHUNK_SIZE * DEFAULT_CHUNK_SIZE
#define DEFAULT_CHUNK_HEIGHT 256

#define TEXTURES_TOTAL 4

typedef unsigned char BlockIndex;

typedef struct BlockType{
    unsigned char textureTop;
    unsigned char textureBottom;
    unsigned char textureLeft;
    unsigned char textureRight;
    unsigned char textureFront;
    unsigned char textureBack; 

    unsigned transparent: 1;
    unsigned untextured: 1;
    unsigned repeatTexture: 1;
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
    Mesh* solidMesh;
    Mesh* transparentMesh;

    GLBuffer solidBuffer;
    GLBuffer transparentBuffer;
} Chunk; 

BlockIndex getChunkBlock(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z);
int setChunkBlock(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z, BlockIndex  value);

Chunk* generatePlainChunk(BlockIndex  top, BlockIndex  rest);
Chunk* generatePerlinChunk(int chunkX, int chunkZ);

Vertex createVertex(float x, float y, float z);
void generateMeshForChunk(Mesh* solid, Mesh* transparent, Chunk* chunk);

#include <world.h>

#endif