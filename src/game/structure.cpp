#include <game/structure.hpp>

std::tuple<SparseBlockArray*, glm::ivec3> Structure::getBlockArrayForPosition(glm::ivec3 position){
    if(
        position.x < 0 || position.x > width  ||
        position.y < 0 || position.y > height ||
        position.z < 0 || position.z > depth
    ) return {nullptr,{0,0,0}};

    glm::ivec3 chunk_position = glm::floor(glm::vec3(position) / 64.0f);
    //std::cout << chunk_position.x << " " << chunk_position.y << " " << chunk_position.z << std::endl;
    return {&block_arrays[chunk_position], chunk_position};
}

void Structure::setBlock(glm::ivec3 position, const Block& block){
    auto [block_array,array_position] = getBlockArrayForPosition(position);
    if(!block_array) return;
    
    glm::ivec3 block_position = position - (array_position * 64);
    block_array->setBlock(block_position, block);
}


Block* Structure::getBlock(glm::ivec3 position){
    static std::shared_mutex mutex;
    std::shared_lock lock(mutex);

    glm::ivec3 chunk_position = glm::floor(glm::vec3(position) / 64.0f);
    if(!block_arrays.contains(chunk_position)) return nullptr;

    glm::ivec3 block_position = position - (chunk_position * 64);
    return block_arrays.at(chunk_position).getBlock(block_position);
}   

Structure::PositionSet Structure::place(const glm::ivec3& position ,Terrain& world){
    PositionSet visited{};

    int i = 0;
    for(auto& [chunk_position, block_array]: block_arrays){
        for(int x = 0;x < 64;x++)
        for(int y = 0;y < 64;y++)
        for(int z = 0;z < 64;z++){
            glm::ivec3 block_position = glm::ivec3{x,y,z} + (chunk_position * 64);
            
            Block* block = getBlock(block_position);
            if(!block || block->id == 0) continue;

            glm::ivec3 block_world_position = position + block_position; 
            glm::ivec3 chunk_position = world.blockToChunkPosition(block_world_position);
            if(!visited.contains(chunk_position)) visited.emplace(chunk_position);

            world.setBlock(block_world_position, *block);
        }
    }

    return visited;
}

Structure Structure::capture(const glm::ivec3& position, const glm::ivec3& size, const Terrain& world){
    Structure output{size.x,size.y,size.z};

    int i = -1;
    int total =  (size.x * size.y * size.z);

    for(int x = 0;x < size.x ;x++)
    for(int y = 0;y < size.y;y++)
    for(int z = 0;z < size.z;z++){
        glm::ivec3 block_position = {x,y,z};

        Block* block = world.getBlock(position + block_position);
        if(!block) continue;
        output.setBlock(block_position, *block);
    }

    return output;
}