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

ByteArray Structure::serialize(){
    ByteArray output{};

    output.append<size_t>(width);
    output.append<size_t>(height);
    output.append<size_t>(depth);

    output.append<size_t>(block_arrays.size());
    for(auto& [position, array]: block_arrays){
        output.append<size_t>(position.x);
        output.append<size_t>(position.y);
        output.append<size_t>(position.z);

        array.serialize(output);
    }

    return output;
}

Structure Structure::deserialize(ByteArray& source_array){
    int width  = source_array.read<size_t>();
    int height = source_array.read<size_t>();
    int depth  = source_array.read<size_t>();
    
    Structure output{width,height,depth};

    int arrays_total = source_array.read<size_t>();
    for(int i = 0;i < arrays_total;i++){
        glm::ivec3 position = {
            source_array.read<size_t>(),
            source_array.read<size_t>(),
            source_array.read<size_t>()
        };

        output.block_arrays[position] = SparseBlockArray::deserialize(source_array);
    }

    return output;
}