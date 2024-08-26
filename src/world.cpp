#include <world.hpp>

std::shared_mutex chunkGenLock;

Chunk* World::generateAndGetChunk(int x, int z){
    const glm::vec2 key = glm::vec2(x,z);

    {
        std::shared_lock lock(chunkGenLock);
        auto it = this->chunks.find(key);
        if (it != this->chunks.end()) {
            return &it->second;
        } 
    }
    
    
    Chunk chunk = generateTerrainChunk(*this,x,z);
    std::unique_lock lock(chunkGenLock);
    auto [iter, success] = this->chunks.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(key),
        std::forward_as_tuple(std::move(chunk))
    );

    return &iter->second;
}

Chunk* World::getChunk(int x, int z){
    const glm::vec2 key = glm::vec2(x,z);
    
    std::shared_lock lock(chunkGenLock);

    auto it = this->chunks.find(key);
    if (it != this->chunks.end()) {
        return &it->second;
    }

    return nullptr;
}

std::size_t Vec2Hash::operator()(const glm::vec2& v) const noexcept{
    std::size_t h1 = std::hash<float>{}(v.x);
    std::size_t h2 = std::hash<float>{}(v.y);
    // Combine the hash values
    return h1 ^ (h2 << 1);
}

bool Vec2Equal::operator()(const glm::vec2& lhs, const glm::vec2& rhs) const noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}



static int threadsTotal = 0;
void generateChunkMeshThread(Chunk* chunk){

    //struct timespec start, end;
    
     //Start timer
    //clock_gettime(CLOCK_REALTIME, &start);

    chunk->generateMeshes();
    // End timer
    //clock_gettime(CLOCK_REALTIME, &end);

    // Calculate time difference
    //double elapsed_time = (end.tv_sec - start.tv_sec) +
    //                     (end.tv_nsec - start.tv_nsec) / 1e9;

    //printf("Time generate chunk mesh: %fs\n", elapsed_time);

    chunk->meshGenerating = false;
    chunk->meshGenerated = true;
    threadsTotal--;
}

Chunk* World::getChunkWithMesh(int x, int y){
    Chunk* chunk = this->getChunk(x, y);
    if(!chunk) return nullptr;

    if(!chunk->meshGenerated && !chunk->meshGenerating && threadsTotal < 16){
        threadsTotal++;
        std::thread t1(generateChunkMeshThread, chunk);
        chunk->meshGenerating = true;

        t1.detach();
        return nullptr;
    }

    if(!chunk->buffersLoaded && chunk->meshGenerated){
        if(!chunk->solidBuffer){
            chunk->solidBuffer = std::make_unique<GLDoubleBuffer>();
        }
        //printf("Loading meshes %2i:%2i (%i)...\n",x,z,chunk->buffersLoaded);
        if (chunk->solidBuffer && chunk->solidMesh.has_value()) {
            GLDoubleBuffer& dbuffer = *chunk->solidBuffer;
            dbuffer.getBackBuffer().loadMesh(chunk->solidMesh.value());
            chunk->solidMesh.reset();
            dbuffer.swap();
            
            chunk->buffersLoaded = true;
            chunk->isDrawn = true;
        }
        else{
            std::cerr << "NAH FAM" << std::endl;
        }
        //printf("Vertices:%i Indices:%i\n", chunk->solidMesh->vertices_count, chunk->solidMesh->indices_count);
        
        return nullptr;
    }

   return chunk;
}

CollisionCheckResult World::checkForPointCollision(float x, float y, float z, bool includeRectangularColliderLess){
    CollisionCheckResult result = {nullptr, false, 0,0,0};
    int range = 3;

    float blockWidth = 1;

    for(int i = -range;i <= range;i++){
        for(int j = -range;j <= range;j++){
            for(int g = -range;g <= range;g++){
                int cx = x + i;
                int cy = y + j;
                int cz = z + g;

                Block* blocki = this->getBlock(cx, cy, cz);
                if(blocki){
                    BlockType block = getBlockType(blocki);
                    if((block.colliders.size() == 0 || blocki->type == BlockTypes::Air) && !includeRectangularColliderLess) continue;

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
                else{
                    result.collision = 1;
                    result.x = cx;
                    result.y = cy;
                    result.z = cz;
                    return result;
                }
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
                    BlockType block = getBlockType(blocki);
                    if(block.colliders.size() == 0 || blocki->type == BlockTypes::Air) continue;

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
                else{
                    result.collision = 1;
                    result.x = cx;
                    result.y = cy;
                    result.z = cz;
                    return result;
                }
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
    if(y < 0 || y > DEFAULT_CHUNK_HEIGHT) return nullptr;
    
    int chunkX = floor((double)x / (double)DEFAULT_CHUNK_SIZE);
    int chunkZ = floor((double)z / (double)DEFAULT_CHUNK_SIZE);

    int ix = abs(x - chunkX * DEFAULT_CHUNK_SIZE);
    int iz = abs(z - chunkZ * DEFAULT_CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %i(%i)x%ix%i(%i)\n", chunkX, chunkZ, ix,y,iz);

    Chunk* chunk = this->getChunk(chunkX, chunkZ);
    if(!chunk) return this->generateAndGetChunk(chunkX, chunkZ)->getBlock(ix,y,iz);

    return chunk->getBlock(ix, y, iz);
}

Chunk* World::getChunkFromBlockPosition(int x, int z){
    int cx,cz;
    if(x >= 0) cx = x / DEFAULT_CHUNK_SIZE;
    else cx = x / DEFAULT_CHUNK_SIZE - 1;
    if(z >= 0) cz = z / DEFAULT_CHUNK_SIZE;
    else cz = z / DEFAULT_CHUNK_SIZE - 1;

    return this->getChunk(cx, cz);
}

bool World::setBlock(int x, int y, int z, Block index){
    if(y < 0 || y > DEFAULT_CHUNK_HEIGHT) return false;
    
    int chunkX = floor((double)x / (double)DEFAULT_CHUNK_SIZE);
    int chunkZ = floor((double)z / (double)DEFAULT_CHUNK_SIZE);

    int ix = abs(x - chunkX * DEFAULT_CHUNK_SIZE);
    int iz = abs(z - chunkZ * DEFAULT_CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %ix%ix%i\n", chunkX, chunkZ, ix,y,iz);

    //Block* i = getWorldBlock(world, ix, y, iz);

    Chunk* chunk = this->getChunk(chunkX, chunkZ);
    if(!chunk) this->generateAndGetChunk(chunkX, chunkZ)->setBlock(ix,y,iz,index);
    else chunk->setBlock(ix, y, iz, index);

    return true;
}
