#include <game/world/world.hpp>

World::World(std::string filepath): generator(){  
    stream = std::make_unique<WorldStream>(filepath);
    generator.setSeed(stream->getSeed());
    addEntity(Entity(glm::vec3(0,60,0), glm::vec3(0.6, 1.8, 0.6))); // Player
}

std::shared_mutex chunkGenLock;

Chunk* World::generateChunk(glm::ivec3 position){
    {
        std::shared_lock lock(chunkGenLock);
        auto it = this->chunks.find(position);
        if (it != this->chunks.end()) return it->second.get(); 
    }

    std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>(position);
    
    generator.generateTerrainChunkAccelerated(chunk.get(),position);

    std::unique_lock lock(chunkGenLock);
    this->chunks.emplace(position, std::move(chunk));
    this->stream->save(*chunks[position]);

    return this->chunks[position].get();
}

Chunk* World::createEmptyChunk(glm::ivec3 position){
    {
        std::shared_lock lock(chunkGenLock);
        auto it = this->chunks.find(position);
        if (it != this->chunks.end()) return it->second.get(); 
    }

    std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>(position);
    std::unique_lock lock(chunkGenLock);
    this->chunks.emplace(position, std::move(chunk));
    this->stream->save(*chunks[position]);

    return this->chunks[position].get();
}

Chunk* World::getChunk(glm::ivec3 position) const {
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
    chunks[position] = std::make_unique<Chunk>(position);
    stream->load(chunks[position].get());
}

std::tuple<bool, Block*> World::checkForPointCollision(glm::vec3 position, bool includeRectangularColliderLess){
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
                    auto* definition = global_block_registry.getBlockPrototypeByIndex(blocki->id);
                    if(!definition) continue;
                    if((definition->colliders.size() == 0 || blocki->id == BLOCK_AIR_INDEX) && !includeRectangularColliderLess) continue;

                    if(
                        position.x >= cx && position.x <= cx + blockWidth &&
                        position.y >= cy && position.y <= cy + blockWidth &&
                        position.z >= cz && position.z <= cz + blockWidth 
                    ) return {true, blocki};
                }
            }
        }
    }

    return {false, nullptr};
}

bool World::collidesWith(glm::vec3 position, Entity* checked_entity, bool vertical_check) {
    int range = 1;

    const RectangularCollider* collider = &checked_entity->getCollider();

    glm::ivec3 ranges = {
        glm::ceil(collider->width),
        glm::ceil(collider->height),
        glm::ceil(collider->depth)
    }; 

    for(int i = -ranges.x;i <= ranges.x;i++){
        for(int j = -ranges.y;j <= ranges.y;j++){
            for(int g = -ranges.z;g <= ranges.z;g++){

                int cx = (int)floor(position.x + i);
                int cy = (int)floor(position.y + j);
                int cz = (int)floor(position.z + g);

                Block* blocki = this->getBlock({cx, cy, cz});
                if(blocki){
                    auto* definition = global_block_registry.getBlockPrototypeByIndex(blocki->id);
                    if(!definition) continue;
                    if(definition->colliders.size() == 0 || blocki->id == BLOCK_AIR_INDEX) continue;

                    for(int colliderIndex = 0;colliderIndex < definition->colliders.size(); colliderIndex++){
                        RectangularCollider* blockCollider = &definition->colliders[colliderIndex];
      
                        if(
                            blockCollider->collidesWith(collider, {cx,cy,cz}, position)
                        ){
                            if(vertical_check) checked_entity->setOnGround(true);
                            return true;
                        }
                    }
                }
            }
        }
    }
    
    if(!checked_entity->isSolid()) return false;

    for(auto& entity: entities){
        if(&entity == checked_entity) continue;

        auto& entity_collider = entity.getCollider();
        auto& checked_collider = checked_entity->getCollider();

        float distance = glm::distance(entity_collider.center, checked_collider.center);
        if(distance > checked_collider.bounding_sphere_radius + entity_collider.bounding_sphere_radius) continue; // Definitly dont collide
        

        if(entity_collider.collidesWith(&checked_collider, entity.getPosition(), position)){
            if(checked_entity->onCollision) checked_entity->onCollision(checked_entity, &entity);
            if(entity.onCollision) entity.onCollision(&entity, checked_entity);
            
            if(entity.isSolid()) return true;
        }
    }

    return false;
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

        auto [collided, block] = this->checkForPointCollision(current_position, 0);
        if(collided){
            result.hit = true;
            result.hitBlock = block;
            
            result.position = glm::floor(current_position);

            return result;
        }

        distance += step;
    }

    return result;
}

glm::ivec3 World::blockToChunkPosition(glm::ivec3 blockPosition) const{
    return glm::floor(glm::vec3(blockPosition) / static_cast<float>(CHUNK_SIZE));
}

glm::ivec3 World::getGetChunkRelativeBlockPosition(glm::ivec3 position){
    return glm::abs(position - blockToChunkPosition(position) * CHUNK_SIZE);
}

Block* World::getBlock(glm::ivec3 position) const {
    auto chunkPosition = blockToChunkPosition(position);
    auto blockPosition = glm::abs(position - chunkPosition * CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %i(%i)x%ix%i(%i)\n", chunkX, chunkZ, ix,iy,iz);

    Chunk* chunk = this->getChunk(chunkPosition);
    if(!chunk) return nullptr;//this->generateAndGetChunk(chunkX, chunkZ)->getBlock(ix,y,iz);

    return chunk->getBlock(blockPosition);
}

Chunk* World::getChunkFromBlockPosition(glm::ivec3 position) const{
    return this->getChunk(blockToChunkPosition(position));
}

bool World::setBlock(glm::ivec3 position, Block block){
    auto chunkPosition = blockToChunkPosition(position);
    auto blockPosition = glm::abs(position - chunkPosition * CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %ix%ix%i\n", chunkX, chunkZ, ix,y,iz);

    //Block* i = getWorldBlock(world, ix, y, iz);

    Chunk* chunk = this->getChunk(chunkPosition);
    if(!chunk) return false;//this->generateAndGetChunk(chunkX, chunkZ)->setBlock(ix,y,iz,index);
    blocks_altered = true;
    chunk->setBlock(blockPosition, block);

    return true;
}


static int rotation = 0;
static float position = 0;
static float position_mult = 1;

void World::drawEntity(Entity& entity){
    if(!entity.getModel()) return;
    
    entity.getModel()->requestDraw(entity.getPosition() + glm::vec3{0,position,0}, {0.3,0.3,0.3}, {0,rotation,0}, entity.getModel()->getRotationCenterOffset());
}

void World::updateEntities(float deltatime){
    int index = 0;

    for(int index = 0;index < entities.size();){
        Entity& entity = entities[index];

        entity.update(this, blocks_altered, deltatime);
        if(entity.shouldGetDestroyed()){
            entities.erase(entities.begin() + index);
            continue;
        }

        drawEntity(entity);

        index++;
    }

    const float position_addition = 0.002;

    position += position_mult;
    if(position <= 0.1) position_mult = position_addition;
    if(position >= 0.4) position_mult = -position_addition;

    rotation = (rotation + 1) % 360;

    blocks_altered = false;
}
void World::drawEntityColliders(WireframeCubeRenderer& renderer, size_t start_index){
    /*for(auto& [region_position,entities]: entity_regions){
        renderer.setCube(
            start_index++,
            region_position * entity_region_size, 
            glm::vec3{entity_region_size},
            {1.0,0.0,1.0}
        );
        for(auto& entity: entities){
            auto& collider = entity->getCollider();
            renderer.setCube(
                start_index++,
                glm::vec3{collider.x, collider.y, collider.z} + entity->getPosition(), 
                glm::vec3{collider.width, collider.height, collider.depth},
                {1.0,1.0,0}
            );
        }
    }*/

    /*for(auto& region_position: getPlayer().getRegionPositions()){
        renderer.setCube(
            start_index++,
            region_position * entity_region_size, 
            glm::vec3{entity_region_size},
            {1.0,0.0,1.0}
        );
    }*/
}