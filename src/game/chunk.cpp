#include <game/chunk.hpp>

Block::Block(){
    this->type = BlockType::Air;
}
Block::Block(BlockType type): type(type){
    this->type = type;
}