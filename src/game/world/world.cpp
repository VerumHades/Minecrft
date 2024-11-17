#include <game/world/world.hpp>

World::World(std::string filepath){
    stream = std::make_unique<WorldStream>(filepath);
    generator = WorldGenerator(stream->getSeed());
}

std::shared_mutex chunkGenLock;

Chunk* World::generateChunk(int x, int y, int z, int lod){
    const glm::vec3 key = glm::vec3(x,y,z);

    {
        std::shared_lock lock(chunkGenLock);
        auto it = this->chunks.find(key);
        if (it != this->chunks.end()) return it->second.get(); 
    }

    std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>(key);
    
    generator.generateTerrainChunkAccelerated(chunk.get(),x,y,z, lod);

    std::unique_lock lock(chunkGenLock);
    this->chunks.emplace(key, std::move(chunk));
    //this->stream->save(*chunks[key]);

    return this->chunks[key].get();
}

Chunk* World::getChunk(int x, int y, int z){
    const glm::vec3 key = glm::vec3(x,y,z);
    
    std::shared_lock lock(chunkGenLock);

    auto it = this->chunks.find(key);
    if (it != this->chunks.end()) {
        return it->second.get();
    }

    return nullptr;
}

bool World::isChunkLoadable(int x, int y, int z){
    return stream->hasChunkAt({x,y,z});
}
void World::loadChunk(int x, int y, int z){
    const glm::vec3 position = glm::vec3(x,y,z);

    std::unique_lock lock(chunkGenLock);
    chunks[position] = std::make_unique<Chunk>(position);
    //stream->load(chunks[position].get());
}

CollisionCheckResult World::checkForPointCollision(float x, float y, float z, bool includeRectangularColliderLess){
    CollisionCheckResult result = {nullptr, false, 0,0,0};
    int range = 3;

    float blockWidth = 1;

    for(int i = -range;i <= range;i++){
        for(int j = -range;j <= range;j++){
            for(int g = -range;g <= range;g++){
                int cx = (int) x + i;
                int cy = (int) y + j;
                int cz = (int) z + g;

                Block* blocki = this->getBlock(cx, cy, cz);
                if(blocki){
                    BlockDefinition block = getBlockDefinition(blocki);
                    if((block.colliders.size() == 0 || blocki->type == BlockType::Air) && !includeRectangularColliderLess) continue;

                    //printf("x:%i y:%i z:%i ax:%f ay:%f az:%f\n",cx,cy,cz,x,y,z);

                    if(
                        x >= cx && x <= cx + blockWidth &&
                        y >= cy && y <= cy + blockWidth &&
                        z >= cz && z <= cz + blockWidth 
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

CollisionCheckResult World::checkForRectangularCollision(float x, float y, float z, RectangularCollider* collider){
    CollisionCheckResult result = {nullptr, false, 0,0,0};
    int range = 3;

    for(int i = -range;i <= range;i++){
        for(int j = -range;j <= range;j++){
            for(int g = -range;g <= range;g++){
                int cx = (int)floor(x + i);
                int cy = (int)floor(y + j);
                int cz = (int)floor(z + g);

                Block* blocki = this->getBlock(cx, cy, cz);
                if(blocki){
                    BlockDefinition block = getBlockDefinition(blocki);
                    if(block.colliders.size() == 0 || blocki->type == BlockType::Air) continue;

                    //printf("x:%i y:%i z:%i ax:%f ay:%f az:%f\n",cx,cy,cz,x,y,z);

                    for(int colliderIndex = 0;colliderIndex < block.colliders.size(); colliderIndex++){
                        RectangularCollider* blockCollider = &block.colliders[colliderIndex];
                        float colliderX = blockCollider->x + cx;
                        float colliderY = blockCollider->y + cy;
                        float colliderZ = blockCollider->z + cz;
                        
                        //printf("%f %f %f %f %f %f\n", colliderX, colliderY, colliderZ, blockCollider->width, blockCollider->height, blockCollider->depth);
            
                        if(
                            x + collider->x + collider->width  >= colliderX && x + collider->x <= colliderX + blockCollider->width &&
                            y + collider->y + collider->height >= colliderY && y + collider->y <= colliderY + blockCollider->height &&
                            z + collider->z + collider->depth  >= colliderZ && z + collider->z <= colliderZ + blockCollider->depth 
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

RaycastResult World::raycast(float fromX, float fromY, float fromZ, float dirX, float dirY, float dirZ, float maxDistance){
    RaycastResult result = {{},false,0,0,0,0,0,0};
    
    float step = 0.5;
    float distance = 0;

    float x = fromX;
    float y = fromY;
    float z = fromZ;

    while(distance < maxDistance){
        result.lastX = x;
        result.lastY = y;
        result.lastZ = z;

        x += dirX * step;
        y += dirY * step;
        z += dirZ * step;

        CollisionCheckResult check = this->checkForPointCollision(x,y,z, 0);
        if( check.collision){
            result.hit = 1;
            result.hitBlock = check.collidedBlock;
            
            result.x = check.x;
            result.y = check.y;
            result.z = check.z;

            return result;
        }

        distance += step;
    }

    return result;
}

Block* World::getBlock(int x, int y, int z){
    int chunkX = (int) floor((double)x / (double)CHUNK_SIZE);
    int chunkY = (int) floor((double)y / (double)CHUNK_SIZE);
    int chunkZ = (int) floor((double)z / (double)CHUNK_SIZE);

    int ix = abs(x - chunkX * CHUNK_SIZE);
    int iy = abs(y - chunkY * CHUNK_SIZE);
    int iz = abs(z - chunkZ * CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %i(%i)x%ix%i(%i)\n", chunkX, chunkZ, ix,iy,iz);

    Chunk* chunk = this->getChunk(chunkX, chunkY, chunkZ);
    if(!chunk) return nullptr;//this->generateAndGetChunk(chunkX, chunkZ)->getBlock(ix,y,iz);

    return chunk->getBlock(ix, iy, iz);
}

Chunk* World::getChunkFromBlockPosition(int x, int y, int z){
    int chunkX = (int) floor((double)x / (double)CHUNK_SIZE);
    int chunkY = (int) floor((double)y / (double)CHUNK_SIZE);
    int chunkZ = (int) floor((double)z / (double)CHUNK_SIZE);

    return this->getChunk(chunkX, chunkY, chunkZ);
}

glm::vec3 World::getGetChunkRelativeBlockPosition(int x, int y, int z){
    int chunkX = (int) floor((double)x / (double)CHUNK_SIZE);
    int chunkY = (int) floor((double)y / (double)CHUNK_SIZE);
    int chunkZ = (int) floor((double)z / (double)CHUNK_SIZE);

    int ix = abs(x - chunkX * CHUNK_SIZE);
    int iy = abs(y - chunkY * CHUNK_SIZE);
    int iz = abs(z - chunkZ * CHUNK_SIZE);

    return glm::vec3((float) ix,(float) iy,(float) iz);
}

bool World::setBlock(int x, int y, int z, Block index){
    int chunkX = (int) floor((double)x / (double)CHUNK_SIZE);
    int chunkY = (int) floor((double)y / (double)CHUNK_SIZE);
    int chunkZ = (int) floor((double)z / (double)CHUNK_SIZE);

    int ix = abs(x - chunkX * CHUNK_SIZE);
    int iy = abs(y - chunkY * CHUNK_SIZE);
    int iz = abs(z - chunkZ * CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %ix%ix%i\n", chunkX, chunkZ, ix,y,iz);

    //Block* i = getWorldBlock(world, ix, y, iz);

    Chunk* chunk = this->getChunk(chunkX, chunkY ,chunkZ);
    if(!chunk) return false;//this->generateAndGetChunk(chunkX, chunkZ)->setBlock(ix,y,iz,index);
    chunk->setBlock(ix, iy, iz, index);

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