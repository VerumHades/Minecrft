#include <game/world/world.hpp>

World::World(std::string filepath, BlockRegistry& blockRegistry): blockRegistry(blockRegistry), generator(blockRegistry){  
    stream = std::make_unique<WorldStream>(filepath);
    generator.setSeed(stream->getSeed());
}

std::shared_mutex chunkGenLock;

Chunk* World::generateChunk(glm::ivec3 position){
    {
        std::shared_lock lock(chunkGenLock);
        auto it = this->chunks.find(position);
        if (it != this->chunks.end()) return it->second.get(); 
    }

    std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>(position, blockRegistry);
    
    generator.generateTerrainChunkAccelerated(chunk.get(),position);

    std::unique_lock lock(chunkGenLock);
    this->chunks.emplace(position, std::move(chunk));
    //this->stream->save(*chunks[key]);

    return this->chunks[position].get();
}

Chunk* World::getChunk(glm::ivec3 position){
    std::shared_lock lock(chunkGenLock);

    auto it = this->chunks.find(position);
    if (it != this->chunks.end()) {
        return it->second.get();
    }

    return nullptr;
}

bool World::isChunkLoadable(glm::ivec3 position){
    return stream->hasChunkAt(position);
}
void World::loadChunk(glm::ivec3 position){
    std::unique_lock lock(chunkGenLock);
    chunks[position] = std::make_unique<Chunk>(position,  blockRegistry);
    //stream->load(chunks[position].get());
}

CollisionCheckResult World::checkForPointCollision(glm::vec3 position, bool includeRectangularColliderLess){
    CollisionCheckResult result = {nullptr, false, 0,0,0};
    int range = 1;

    float blockWidth = 1;

    for(int i = -range;i <= range;i++){
        for(int j = -range;j <= range;j++){
            for(int g = -range;g <= range;g++){
                int cx = (int) position.x + i;
                int cy = (int) position.y + j;
                int cz = (int) position.z + g;

                Block* blocki = this->getBlock({cx, cy, cz});
                if(blocki){
                    auto* definition = blockRegistry.getRegisteredBlockByIndex(blocki->id);
                    if(!definition) continue;
                    if((definition->colliders.size() == 0 || blocki->id == BLOCK_AIR_INDEX) && !includeRectangularColliderLess) continue;

                    //printf("x:%i y:%i z:%i ax:%f ay:%f az:%f\n",cx,cy,cz,x,y,z);

                    if(
                        position.x >= cx && position.x <= cx + blockWidth &&
                        position.y >= cy && position.y <= cy + blockWidth &&
                        position.z >= cz && position.z <= cz + blockWidth 
                    ){
                        result.collidedBlock = blocki;
                        result.collision = 1;
                        result.x = cx;
                        result.y = cy;
                        result.z = cz;
                        return result;
                    }
                }
                /*else{
                    result.collision = 1;
                    result.x = cx;
                    result.y = cy;
                    result.z = cz;
                    return result;
                }*/
            }
        }
    }

    return result;
}

CollisionCheckResult World::checkForRectangularCollision(glm::vec3 position, RectangularCollider* collider){
    CollisionCheckResult result = {nullptr, false, 0,0,0};
    int range = 1;

    for(int i = -range;i <= range;i++){
        for(int j = -range;j <= range;j++){
            for(int g = -range;g <= range;g++){
                int cx = (int)floor(position.x + i);
                int cy = (int)floor(position.y + j);
                int cz = (int)floor(position.z + g);

                Block* blocki = this->getBlock({cx, cy, cz});
                if(blocki){
                    auto* definition = blockRegistry.getRegisteredBlockByIndex(blocki->id);
                    if(!definition) continue;
                    if(definition->colliders.size() == 0 || blocki->id == BLOCK_AIR_INDEX) continue;

                    //printf("x:%i y:%i z:%i ax:%f ay:%f az:%f\n",cx,cy,cz,x,y,z);

                    for(int colliderIndex = 0;colliderIndex < definition->colliders.size(); colliderIndex++){
                        RectangularCollider* blockCollider = &definition->colliders[colliderIndex];
                        float colliderX = blockCollider->x + cx;
                        float colliderY = blockCollider->y + cy;
                        float colliderZ = blockCollider->z + cz;
                        
                        //printf("%f %f %f %f %f %f\n", colliderX, colliderY, colliderZ, blockCollider->width, blockCollider->height, blockCollider->depth);
            
                        if(
                            position.x + collider->x + collider->width  >= colliderX && position.x + collider->x <= colliderX + blockCollider->width &&
                            position.y + collider->y + collider->height >= colliderY && position.y + collider->y <= colliderY + blockCollider->height &&
                            position.z + collider->z + collider->depth  >= colliderZ && position.z + collider->z <= colliderZ + blockCollider->depth 
                        ){
                            result.collidedBlock = blocki;
                            result.collision = 1;
                            result.x = cx;
                            result.y = cy;
                            result.z = cz;
                            return result;
                        }
                    }
                }
                /*else{
                    result.collision = 1;
                    result.x = cx;
                    result.y = cy;
                    result.z = cz;
                    return result;
                }*/
            }
        }
    }

    return result;
}

RaycastResult World::raycast(glm::vec3 from, glm::vec3 direction, float maxDistance){
    RaycastResult result = {{},false,{0,0,0},{0,0,0}};
    
    direction = glm::normalize(direction);

    float step = 0.5;
    float distance = 0;

    glm::vec3 current_position = from;

    while(distance < maxDistance){
        result.lastPosition = current_position;

        current_position += direction * step;

        CollisionCheckResult check = this->checkForPointCollision(current_position, 0);
        if(check.collision){
            result.hit = true;
            result.hitBlock = check.collidedBlock;
            
            result.position = glm::floor(current_position);

            return result;
        }

        distance += step;
    }

    return result;
}

glm::ivec3 World::blockToChunkPosition(glm::ivec3 blockPosition){
    return glm::floor(glm::vec3(blockPosition) / static_cast<float>(CHUNK_SIZE));
}

glm::ivec3 World::getGetChunkRelativeBlockPosition(glm::ivec3 position){
    return glm::abs(position - blockToChunkPosition(position) * CHUNK_SIZE);
}

Block* World::getBlock(glm::ivec3 position){
    auto chunkPosition = blockToChunkPosition(position);
    auto blockPosition = glm::abs(position - chunkPosition * CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %i(%i)x%ix%i(%i)\n", chunkX, chunkZ, ix,iy,iz);

    Chunk* chunk = this->getChunk(chunkPosition);
    if(!chunk) return nullptr;//this->generateAndGetChunk(chunkX, chunkZ)->getBlock(ix,y,iz);

    return chunk->getBlock(blockPosition);
}

Chunk* World::getChunkFromBlockPosition(glm::ivec3 position){
    return this->getChunk(blockToChunkPosition(position));
}

bool World::setBlock(glm::ivec3 position, Block block){
    auto chunkPosition = blockToChunkPosition(position);
    auto blockPosition = glm::abs(position - chunkPosition * CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %ix%ix%i\n", chunkX, chunkZ, ix,y,iz);

    //Block* i = getWorldBlock(world, ix, y, iz);

    Chunk* chunk = this->getChunk(chunkPosition);
    if(!chunk) return false;//this->generateAndGetChunk(chunkX, chunkZ)->setBlock(ix,y,iz,index);
    chunk->setBlock(blockPosition, block);

    return true;
}

void World::drawEntities(ModelManager& manager, Camera& camera, bool depthMode){
    for (auto& entity: this->entities) { 
        if(entity.getModelName() == "default") continue;
        
        manager.drawModel(manager.getModel(entity.getModelName()), camera, entity.getPosition(), depthMode);
    }
}

void World::updateEntities(){
    for (auto& entity: this->entities) { 
        entity.update(*this);
    }
}