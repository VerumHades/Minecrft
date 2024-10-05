#ifndef BLOCKS_H
#define BLOCKS_H

#include <vector>

struct RectangularCollider {
    float x, y, z;
    float width, height, depth;
};

enum class BlockTypes {
    Air,
    Grass,
    Dirt,
    Stone,
    LeafBlock,
    OakLog,
    BirchLeafBlock,
    BirchLog,
    BlueWool,
    Sand,
    BLOCK_TYPES_TOTAL
};

typedef struct Block{
    BlockTypes type;
    

    Block();
    Block(BlockTypes type);
} Block;

struct BlockType {
    bool transparent = false;
    bool untextured = false;
    bool repeatTexture = false;
    std::vector<unsigned char> textures = {};
    std::vector<RectangularCollider> colliders = {{0, 0, 0, 1.0f, 1.0f, 1.0f}};

    // Constructor for convenience
    BlockType(bool transparent = false, bool untextured = false, bool repeatTexture = false,
              std::vector<unsigned char> textures = {}, std::vector<RectangularCollider> colliders = {})
        : transparent(transparent), untextured(untextured), repeatTexture(repeatTexture),
          textures(std::move(textures)) {}
};

#endif