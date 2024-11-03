#include <game/blocks.hpp>

std::unordered_map<BlockTypes, BlockType> predefinedBlocks = {
    {BlockTypes::Air            , BlockType(true , false)}, // Air
    {BlockTypes::Grass          , BlockType(false, true , false ,false, {0, 2, 1, 1, 1, 1})}, // Grass
    {BlockTypes::Dirt           , BlockType(false, true , false ,true, {2})}, // Dirt
    {BlockTypes::Stone          , BlockType(false, true , false ,true, {3})}, // Stone
    {BlockTypes::LeafBlock      , BlockType(false, true , false ,true, {6})}, // Leaf Block
    {BlockTypes::OakLog         , BlockType(false, true , false ,false, {4, 4, 5, 5, 5, 5})}, // Oak Log
    {BlockTypes::BirchLeafBlock , BlockType(false, true , false ,true, {7})}, // Birch Leaf Block
    {BlockTypes::BirchLog       , BlockType(false, true , false ,false, {9, 9, 8, 8, 8, 8})}, // Birch Log
    {BlockTypes::BlueWool       , BlockType(false, true , false ,true, {10})}, // Blue Wool
    {BlockTypes::Sand           , BlockType(false, true , false ,true, {11})}, // Sand
    {BlockTypes::GrassBillboard , BlockType(false, false, true  ,false,{12})}
}; 

#define BLT(name) case BlockTypes::name: return #name;
std::string getBlockTypeName(BlockTypes type){
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