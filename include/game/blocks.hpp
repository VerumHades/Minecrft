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
    GrassBillboard,
    BLOCK_TYPES_TOTAL
};

typedef struct Block{
    BlockTypes type;

    Block();
    Block(BlockTypes type);
} Block;

struct BlockType {
    bool transparent = false;
    bool nonSolid = false;
    bool repeatTexture = false;
    bool billboard = false;
    std::vector<unsigned char> textures = {};
    std::vector<RectangularCollider> colliders = {{0, 0, 0, 1.0f, 1.0f, 1.0f}};

    // Constructor for convenience
    BlockType(bool transparent = false, bool nonSolid = false, bool billboard = false, bool repeatTexture = false,
              std::vector<unsigned char> textures = {}, std::vector<RectangularCollider> colliders = {})
        : transparent(transparent), nonSolid(nonSolid), repeatTexture(repeatTexture),
          textures(std::move(textures)), billboard(billboard) {}
};


extern std::unordered_map<BlockTypes, BlockType> predefinedBlocks;

inline const BlockType& getBlockType(Block* block){
    if (block->type < BlockTypes::Air || block->type >= BlockTypes::BLOCK_TYPES_TOTAL) {
        std::cerr << "Error: Invalid BlockTypes value: " << static_cast<int>(block->type) << std::endl;
        std::terminate(); 
    }

    return predefinedBlocks[block->type];
}

std::string getBlockTypeName(BlockTypes type);

#endif