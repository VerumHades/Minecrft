#ifndef CHUNK_H
#define CHUNK_H

#include <cmath>
#include <ctime>

#include <rendering/texture.hpp>
#include <rendering/buffer.hpp>
#include <rendering/mesh.hpp>

#define OK -1
#define INVALID_COORDINATES -2
#define LAYER_CORRUPTED -3

#define DEFAULT_CHUNK_SIZE 16
#define DEFAULT_CHUNK_AREA DEFAULT_CHUNK_SIZE * DEFAULT_CHUNK_SIZE
#define DEFAULT_CHUNK_HEIGHT 256

#define TEXTURES_TOTAL 4

extern float textureSize;
typedef struct Block{
    unsigned int typeIndex: 5;
} __attribute__((packed)) Block;

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

typedef struct Chunk{
    int worldX;
    int worldZ;
    struct World* world;
    unsigned stored: 1; // If chunks is stored in world file

    Block blocks[DEFAULT_CHUNK_SIZE][DEFAULT_CHUNK_HEIGHT][DEFAULT_CHUNK_SIZE];
    //unsigned char lightArray[DEFAULT_CHUNK_SIZE][DEFAULT_CHUNK_HEIGHT][DEFAULT_CHUNK_SIZE][3];

    unsigned meshGenerating: 1;
    unsigned meshGenerated: 1;

    unsigned buffersLoaded: 1;
    unsigned buffersAsigned: 1;

    unsigned isDrawn: 1;
    
    Mesh* solidMesh;
    Mesh* transparentMesh;

    //unsigned lightTextureLoaded: 1;
    //GLTexture3D lightTexture;

    GLBuffer solidBuffer;
    GLBuffer solidBackBuffer;

    GLBuffer transparentBuffer;
    GLBuffer transparentBackBuffer;
} Chunk; 

typedef struct FaceDefinition{
    int offsetX; int offsetY; int offsetZ;
    int* vertexIndexes;
    int textureIndex;
    int clockwise;
} FaceDefinition;

typedef struct World World;
extern BlockType predefinedBlocks[];

void destroyChunk(Chunk* chunk);

Block* getChunkBlock(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z);
int setChunkBlock(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z, Block  value);

Chunk* generateEmptyChunk(World* world);
Chunk* generatePlainChunk(World* world, Block  top, Block  rest);
Chunk* generatePerlinChunk(World* world, int chunkX, int chunkZ);

void generateMeshForChunk(Mesh* solid, Mesh* transparent, Chunk* chunk);

#include <world.hpp>

#endif