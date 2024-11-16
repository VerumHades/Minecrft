#include <game/blocks.hpp>

std::unordered_map<BlockType, BlockDefinition> predefinedBlocks = {
    {BlockType::Air            , BlockDefinition(true , false)}, // Air
    {BlockType::Grass          , BlockDefinition(false, true , false ,false, {0, 2, 1, 1, 1, 1})}, // Grass
    {BlockType::Dirt           , BlockDefinition(false, true , false ,true, {2})}, // Dirt
    {BlockType::Stone          , BlockDefinition(false, true , false ,true, {3})}, // Stone
    {BlockType::LeafBlock      , BlockDefinition(false, true , false ,true, {6})}, // Leaf Block
    {BlockType::OakLog         , BlockDefinition(false, true , false ,false, {4, 4, 5, 5, 5, 5})}, // Oak Log
    {BlockType::BirchLeafBlock , BlockDefinition(false, true , false ,true, {7})}, // Birch Leaf Block
    {BlockType::BirchLog       , BlockDefinition(false, true , false ,false, {9, 9, 8, 8, 8, 8})}, // Birch Log
    {BlockType::BlueWool       , BlockDefinition(false, true , false ,true, {10})}, // Blue Wool
    {BlockType::Sand           , BlockDefinition(false, true , false ,true, {11})}, // Sand
    {BlockType::GrassBillboard , BlockDefinition(false, false, true  ,false,{12})}
}; 

#define BLT(name) case BlockType::name: return #name;
std::string getBlockTypeName(BlockType type){
    switch(type){
        BLT(Air)
        BLT(Grass) 
        BLT(Dirt)
        BLT(Stone)
        BLT(LeafBlock)
        BLT(OakLog)
        BLT(BirchLeafBlock)
        BLT(BirchLog)
        BLT(BlueWool)
        BLT(Sand)
        default: return "Unknown?";
    }
}
#undef BLT