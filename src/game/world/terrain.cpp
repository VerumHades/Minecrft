#include <game/world/terrain.hpp>

std::shared_mutex chunkGenLock;

Chunk* Terrain::createEmptyChunk(glm::ivec3 position){
    {
        std::shared_lock lock(chunkGenLock);
        auto it = this->chunks.find(position);
        if (it != this->chunks.end()) return it->second.get(); 
    }

    std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>(position);
    std::unique_lock lock(chunkGenLock);
    this->chunks.emplace(position, std::move(chunk));

    return this->chunks[position].get();
}

void Terrain::removeChunk(const glm::ivec3& position){
    std::unique_lock lock(chunkGenLock);
    //if(this->chunks.contains(position))
    //    this->chunks.erase(position);
}

Chunk* Terrain::getChunk(glm::ivec3 position) const {
    std::shared_lock lock(chunkGenLock);

    auto it = this->chunks.find(position);
    if (it != this->chunks.end()) {
        return it->second.get();
    }

    return nullptr;
}

bool Terrain::collision(glm::vec3 position, const RectangularCollider* collider){
    glm::ivec3 ranges = {
        glm::ceil(collider->width),
        glm::ceil(collider->height),
        glm::ceil(collider->depth)
    }; 

    for(int i = -ranges.x;i <= ranges.x;i++){
        for(int j = -ranges.y;j <= ranges.y;j++){
            for(int g = -ranges.z;g <= ranges.z;g++){
                
                glm::ivec3 real_position = {
                    floor(position.x + i),
                    floor(position.y + j),
                    floor(position.z + g)
                };

                Block* blocki = getBlock(real_position);
                if(!blocki) continue;

                auto* definition = BlockRegistry::get().getPrototype(blocki->id);
                if(!definition) continue;
                
                if(definition->colliders.size() == 0 || blocki->id == BLOCK_AIR_INDEX) continue;

                for(auto& blockCollider: definition->colliders){
                    if(
                        !blockCollider.collidesWith(collider, real_position, position)
                    ) continue;

                    return true;
                }
                
            }
        }
    }

    return false;
}

/*
    Takes a local_distance, which is the in block distance from the floored block position and a direction part from a vector, 
    decides the ratio for that direction and distance
*/
static inline float findDeltaRatio(float direction, float local_distance){
    float upper_half = local_distance;
    float lower_half = 1.0f - upper_half;

    if(abs(direction) < 0.0001f) return std::numeric_limits<float>::max();
    return direction > 0.0f  ? (lower_half / direction) : (upper_half / -direction);
}

RaycastResult Terrain::raycast(const glm::vec3& from, const glm::vec3& direction, float max_distance){
    glm::vec3 current_position = from;
    glm::vec3 block_position, ratios, local_position, last_block_position, current_movement;

    int selected_ratio = 0;
    float ratio = 0.0f;
    float current_distance = 0.0f;
    float distance = 0.0f;

    int i = 1;
    while(true){
        block_position = glm::floor(current_position);

        auto* block = getBlock(block_position);
        if(block && block->id != BLOCK_AIR_INDEX) return {block_position, last_block_position};

        local_position = glm::abs(current_position - block_position);

        ratios = {
            findDeltaRatio(direction.x, local_position.x),
            findDeltaRatio(direction.y, local_position.y),
            findDeltaRatio(direction.z, local_position.z)
        };

        ratio = glm::min(glm::min(ratios.x, ratios.y), ratios.z) + 0.001f; // Slight offset to not get stuck across block boundaries

        current_movement = direction * ratio;
        current_position += current_movement;
        
        distance = glm::length(current_movement);

        current_distance += distance;
        if(current_distance >= max_distance)
            return {block_position, last_block_position};
        
        last_block_position = block_position;
    }
}

glm::ivec3 Terrain::blockToChunkPosition(glm::ivec3 blockPosition) const{
    return glm::floor(glm::vec3(blockPosition) / static_cast<float>(CHUNK_SIZE));
}

glm::ivec3 Terrain::getGetChunkRelativeBlockPosition(glm::ivec3 position){
    return glm::abs(position - blockToChunkPosition(position) * CHUNK_SIZE);
}

Block* Terrain::getBlock(glm::ivec3 position) const {
    auto chunkPosition = blockToChunkPosition(position);
    auto blockPosition = glm::abs(position - chunkPosition * CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %i(%i)x%ix%i(%i)\n", chunkX, chunkZ, ix,iy,iz);

    Chunk* chunk = this->getChunk(chunkPosition);
    if(!chunk) return nullptr;//this->generateAndGetChunk(chunkX, chunkZ)->getBlock(ix,y,iz);

    return chunk->getBlock(blockPosition);
}

Chunk* Terrain::getChunkFromBlockPosition(glm::ivec3 position) const{
    return this->getChunk(blockToChunkPosition(position));
}

bool Terrain::setBlock(const glm::ivec3& position, const Block& block){
    auto chunkPosition = blockToChunkPosition(position);
    auto blockPosition = glm::abs(position - chunkPosition * CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %ix%ix%i\n", chunkX, chunkZ, ix,y,iz);

    //Block* i = getWorldBlock(world, ix, y, iz);

    Chunk* chunk = this->getChunk(chunkPosition);
    if(!chunk) return false;//this->generateAndGetChunk(chunkX, chunkZ)->setBlock(ix,y,iz,index);
    chunk->setBlock(blockPosition, block);

    return true;
}