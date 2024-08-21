#ifndef CHUNK_H
#define CHUNK_H

#include <cmath>
#include <ctime>

#include <rendering/texture.hpp>
#include <rendering/buffer.hpp>
#include <rendering/mesh.hpp>
#include <glm/glm.hpp>
#include <map>
#include <optional>
#include <functional>

#define OK -1
#define INVALID_COORDINATES -2
#define LAYER_CORRUPTED -3

#define DEFAULT_CHUNK_SIZE 16
#define DEFAULT_CHUNK_AREA DEFAULT_CHUNK_SIZE * DEFAULT_CHUNK_SIZE
#define DEFAULT_CHUNK_HEIGHT 256

#define TEXTURES_TOTAL 4

extern float textureSize;

struct RectangularCollider {
    float x, y, z;
    float width, height, depth;
};

enum class BlockTypeEnum {
    Air,
    Grass,
    Dirt,
    Stone,
    LeafBlock,
    OakLog,
    BirchLeafBlock,
    BirchLog,
    BlueWool,
    Sand
};

typedef struct Block{
    BlockTypeEnum type;
} Block;

struct BlockType {
    bool transparent = false;
    bool untextured = false;
    bool repeatTexture = false;
    std::vector<unsigned char> textures = {};
    std::vector<RectangularCollider> colliders;

    // Constructor for convenience
    BlockType(bool transparent = false, bool untextured = false, bool repeatTexture = false,
              std::vector<unsigned char> textures = {}, std::vector<RectangularCollider> colliders = {})
        : transparent(transparent), untextured(untextured), repeatTexture(repeatTexture),
          textures(std::move(textures)), colliders(std::move(colliders)) {}
};

struct FaceDefinition {
    int offsetX = 0;
    int offsetY = 0;
    int offsetZ = 0;
    std::vector<int> vertexIndexes;
    int textureIndex = 0;
    bool clockwise = false;

    FaceDefinition(int offsetX, int offsetY, int offsetZ, 
                   std::vector<int> vertexIndexes, int textureIndex, 
                   bool clockwise = false)
        : offsetX(offsetX), offsetY(offsetY), offsetZ(offsetZ),
          vertexIndexes(std::move(vertexIndexes)), textureIndex(textureIndex),
          clockwise(clockwise) {}
};


class World;

class Chunk{
    private:
        glm::vec2 worldPosition;
        World& world;

        Block blocks[DEFAULT_CHUNK_SIZE][DEFAULT_CHUNK_HEIGHT][DEFAULT_CHUNK_SIZE];
        //unsigned char lightArray[DEFAULT_CHUNK_SIZE][DEFAULT_CHUNK_HEIGHT][DEFAULT_CHUNK_SIZE][3];
        
    public:
        bool meshGenerating;
        bool meshGenerated;

        bool buffersLoaded;
        bool buffersAsigned;

        GLBuffer solidBuffer;
        GLBuffer solidBackBuffer;

        GLBuffer transparentBuffer;
        GLBuffer transparentBackBuffer;

        std::optional<Mesh> solidMesh;
        std::optional<Mesh> transparentMesh;

        bool isDrawn;

        Chunk(World& world, const glm::vec2& pos);

        const Block& getBlock(unsigned int x, unsigned int y, unsigned int z);
        void setBlock(unsigned int x, unsigned int y, unsigned int z, Block value);

        void generateMeshes();
        void regenerateMesh();

        const glm::vec2& getWorldPosition();
        World& getWorld();
}; 
extern std::map<BlockTypeEnum, BlockType> predefinedBlocks;

#include <world.hpp>

#endif