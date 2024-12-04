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
    chunks[position] = std::make_unique<Chunk>(position,  blockRegistry);
    //stream->load(chunks[position].get());
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
                    auto* definition = blockRegistry.getBlockPrototypeByIndex(blocki->id);
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

bool World::collidesWith(glm::vec3 position, RectangularCollider* collider) const {
    int range = 1;

    for(int i = -range;i <= range;i++){
        for(int j = -range;j <= range;j++){
            for(int g = -range;g <= range;g++){
                int cx = (int)floor(position.x + i);
                int cy = (int)floor(position.y + j);
                int cz = (int)floor(position.z + g);

                Block* blocki = this->getBlock({cx, cy, cz});
                if(blocki){
                    auto* definition = blockRegistry.getBlockPrototypeByIndex(blocki->id);
                    if(!definition) continue;
                    if(definition->colliders.size() == 0 || blocki->id == BLOCK_AIR_INDEX) continue;

                    //printf("x:%i y:%i z:%i ax:%f ay:%f az:%f\n",cx,cy,cz,x,y,z);

                    for(int colliderIndex = 0;colliderIndex < definition->colliders.size(); colliderIndex++){
                        RectangularCollider* blockCollider = &definition->colliders[colliderIndex];
                        
                        //printf("%f %f %f %f %f %f\n", colliderX, colliderY, colliderZ, blockCollider->width, blockCollider->height, blockCollider->depth);
            
                        if(
                            blockCollider->collidesWith(collider, {cx,cy,cz}, position)
                        ){
                            return true;
                        }
                    }
                }
            }
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
    chunk->setBlock(blockPosition, block);

    return true;
}


static int rotation = 0;
static float position = 0;
static float position_mult = 1;

glm::ivec3 World::getEntityRegionPosition(const std::shared_ptr<Entity>& entity){
    if(!entity) return {0,0,0};
    return glm::floor(entity->getPosition() / static_cast<float>(entity_region_size));
}

void World::drawEntity(const std::shared_ptr<Entity>& entity){
    if(!entity->getModel()) return;
        
    entity->getModel()->requestDraw(entity->getPosition() + glm::vec3{0,position,0}, {0.5,0.5,0.5}, {0,rotation,0}, {-0.5,0,0});
}

void World::addEntityToRegion(const glm::ivec3 region_position, std::shared_ptr<Entity> entity){
    if(!entity_regions.contains(region_position)){
        entity_regions[region_position] = {};
    }

    entity_regions[region_position].push_back(entity);
    entity->getLastRegionPosition().emplace(region_position);  
}
bool World::removeEntityFromRegion(const glm::ivec3 region_position,  std::shared_ptr<Entity> entity){
    if(!entity_regions.contains(region_position)) return false;

    auto& last_region = entity_regions[region_position];

    auto iter = std::find(last_region.begin(), last_region.end(), entity);
    if(iter == last_region.end()) return false;
    last_region.erase(iter);

    entity->getLastRegionPosition().reset();
    
    return true;
}

void World::updateEntity(std::shared_ptr<Entity> entity){  
    glm::ivec3 current_region_position = getEntityRegionPosition(entity);
    auto& last_region_position_opt = entity->getLastRegionPosition();
    
    if(!last_region_position_opt.has_value()) addEntityToRegion(current_region_position, entity);
    else{
        auto last_region_position = last_region_position_opt.value();
        if(current_region_position != last_region_position){
            //removeEntityFromRegion(last_region_position, entity);
            addEntityToRegion(current_region_position, entity);
        }
    }

    entity->update(this);
}

void World::drawEntities(){
    for (auto& [region_position, entity_list]: this->entity_regions) { 
        for(auto& entity: entity_list){
            drawEntity(entity);
        }
    }

    const float position_addition = 0.002;

    position += position_mult;
    if(position <= 0.1) position_mult = position_addition;
    if(position >= 0.4) position_mult = -position_addition;

    rotation = (rotation + 1) % 360;
}

void World::updateEntities(){
    for (auto& [region_position, entity_list]: this->entity_regions) { 
        for(auto& entity: entity_list){
            updateEntity(entity);
        }
    }
}
void World::drawEntityColliders(WireframeCubeRenderer& renderer, size_t start_index){
    for (auto& [region_position, entity_list]: this->entity_regions) { 
        for(auto& entity: entity_list){
            auto& collider = entity->getCollider();
            renderer.setCube(
                start_index++,
                glm::vec3{collider.x, collider.y, collider.z} + entity->getPosition(), 
                glm::vec3{collider.width, collider.height, collider.depth},
                {1.0,0,0}
            );
        }
    }
}