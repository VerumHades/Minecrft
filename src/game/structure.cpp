#include <game/structure.hpp>

std::tuple<SparseBlockArray*, glm::ivec3> Structure::getBlockArrayForPosition(glm::ivec3 position){
    if(
        position.x < 0 || position.x > width  ||
        position.y < 0 || position.y > height ||
        position.z < 0 || position.z > depth
    ) return {nullptr,{0,0,0}};

    glm::ivec3 indexes = glm::floor(glm::vec3(position) / 64.0f);

    return {&block_arrays[indexes.x + (indexes.y * width) + (indexes.z * width * height)], indexes};
}

void Structure::setBlock(glm::ivec3 position, const Block& block){
    auto [block_array,array_position] = getBlockArrayForPosition(position);
    if(!block_array) return;
    glm::ivec3 block_position = position - (array_position * 64);

    block_array->setBlock(block_position, block);
}
Block* Structure::getBlock(glm::ivec3 position){
    auto [block_array,array_position] = getBlockArrayForPosition(position);
    if(!block_array) return nullptr;
    glm::ivec3 block_position = position - (array_position * 64);

    return block_array->getBlock(block_position);
}   

void Structure::place(const glm::ivec3& position ,World& world){
    for(int x = 0;x < width ;x++)
    for(int y = 0;y < height;y++)
    for(int z = 0;z < depth ;z++){
        glm::ivec3 block_position = {x,y,z};
        
        Block* block = getBlock(block_position);
        if(!block) continue;

        glm::ivec3 block_world_position = position + block_position; 
        world.setBlock(block_world_position, *block);
    }
}

Structure Structure::capture(const glm::ivec3& position, const glm::ivec3& size, const World& world){
    Structure output{size.x,size.y,size.z};

    for(int x = 0;x < output.width ;x++)
    for(int y = 0;y < output.height;y++)
    for(int z = 0;z < output.depth ;z++){
        glm::ivec3 block_position = {x,y,z};

        Block* block = world.getBlock(position + block_position);
        if(!block) continue;
        output.setBlock(block_position, *block);
    }

    return output;
}