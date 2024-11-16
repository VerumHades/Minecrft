#include <game/chunk.hpp>

static inline void fillChunkLevel(Chunk& chunk, uint y, Block value){
    for(int x = 0; x < CHUNK_SIZE;x++) for(int z = 0;z < CHUNK_SIZE;z++){
        chunk.setBlock(x,y,z, value);
    }
}

Block::Block(){
    this->type = BlockType::Air;
}
Block::Block(BlockType type): type(type){
    this->type = type;
}