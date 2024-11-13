#include <game/chunk.hpp>

static inline void fillChunkLevel(Chunk& chunk, uint32_t y, Block value){
    for(int x = 0; x < CHUNK_SIZE;x++) for(int z = 0;z < CHUNK_SIZE;z++){
        chunk.setBlock(x,y,z, value);
    }
}

Block::Block(){
    this->type = BlockTypes::Air;
}
Block::Block(BlockTypes type): type(type){
    this->type = type;
}

static Block airBlock = {BlockTypes::Air};
Block* Chunk::getBlock(uint32_t x, uint32_t y, uint32_t z){
    if(x >= CHUNK_SIZE) return nullptr;
    if(y >= CHUNK_SIZE) return nullptr;
    if(z >= CHUNK_SIZE) return nullptr;
    if(!isMainGroupOfSize(64)) return nullptr;
    
    uint64_t checkMask = (1ULL << (63 - x));
    
    for(auto& [key,mask]: currentGroup->getMasks()){
        uint64_t row = mask.getAt(z,y);

        if(row & checkMask){
            return &mask.getBlock();
        }
    }

    return &airBlock;
}

bool Chunk::setBlock(uint32_t x, uint32_t y, uint32_t z, Block value){
    if(x >= CHUNK_SIZE) return false;
    if(y >= CHUNK_SIZE) return false;
    if(z >= CHUNK_SIZE) return false;
    if(!isMainGroupOfSize(64)) return false;

    uint64_t currentMask = (1ULL << (63 - x));

    if(value.type != BlockTypes::Air){
        if(currentGroup->getMasks().count(value.type) == 0){
            DynamicChunkMask mask = {64,value};
            currentGroup->setMask(value.type, mask);
        }

        currentGroup->getMask(value.type).set(x,y,z);
    }

    for(auto& [key,mask]: currentGroup->getMasks()){
        uint64_t row = mask.getAt(z,y);

        if(!(row & currentMask)) continue;
        if(mask.getBlock().type == value.type) continue;

        mask.reset(x,y,z);
        break;
    }

    if(getBlockType(&value).solid) currentGroup->getSolidMask().set(x,y,z);
    else currentGroup->getSolidMask().reset(x,y,z);

    return true;
}