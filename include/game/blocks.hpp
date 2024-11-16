#ifndef BLOCKS_H
#define BLOCKS_H

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <iostream>

struct RectangularCollider {
    float x, y, z;
    float width, height, depth;
};

enum class BlockType {
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
    GrassBillboard,
    BLOCK_TYPES_TOTAL
};

typedef struct Block{
    BlockType type;

    Block();
    Block(BlockType type);
} Block;

struct BlockDefinition {
    bool transparent = false;
    bool solid = false;
    bool repeatTexture = false;
    bool billboard = false;
    std::vector<unsigned char> textures = {};
    std::vector<RectangularCollider> colliders = {{0, 0, 0, 1.0f, 1.0f, 1.0f}};

    // Constructor for convenience
    BlockDefinition(bool transparent = false, bool solid = false, bool billboard = false, bool repeatTexture = false,
              std::vector<unsigned char> textures = {}, std::vector<RectangularCollider> colliders = {})
        : transparent(transparent), solid(solid), repeatTexture(repeatTexture),
          textures(std::move(textures)), billboard(billboard) {}
};


extern std::unordered_map<BlockType, BlockDefinition> predefinedBlocks;

inline const BlockDefinition& getBlockDefinition(Block* block){
    if (block->type < BlockType::Air || block->type >= BlockType::BLOCK_TYPES_TOTAL) {
        std::cerr << "Error: Invalid BlockType value: " << static_cast<int>(block->type) << std::endl;
        std::terminate(); 
    }

    return predefinedBlocks[block->type];
}

std::string getBlockTypeName(BlockType type);

#endif