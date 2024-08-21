#include <world.hpp>
Chunk& World::generateChunk(int x, int z){
    glm::vec2 key = glm::vec2(x,z);

    if(this->chunks.count(key) != 0){
        return this->chunks[glm::vec2(x,z)];
    }
    else{
        this->chunks.emplace(glm::vec2(x,z), Chunk(*this, glm::vec2(x,z)));
        return this->chunks[glm::vec2(x,z)];
    }
}

std::optional<std::reference_wrapper<Chunk>> World::getChunk(int x, int z){
    if(this->chunks.count(glm::vec2(x,z)) != 0){
        return this->chunks[glm::vec2(x,z)];
    }

    return std::nullopt;
}



void generateChunkMeshThread(Chunk& chunk){
    chunk.meshGenerating = 1;
    chunk.meshGenerated = 0;

    //struct timespec start, end;
    
     //Start timer
    //clock_gettime(CLOCK_REALTIME, &start);

    chunk.generateMeshes();

    // End timer
    //clock_gettime(CLOCK_REALTIME, &end);

    // Calculate time difference
    //double elapsed_time = (end.tv_sec - start.tv_sec) +
    //                     (end.tv_nsec - start.tv_nsec) / 1e9;

    //printf("Time generate chunk mesh: %fs\n", elapsed_time);

    chunk.meshGenerating = 0;
    chunk.meshGenerated = 1;
}

std::optional<std::reference_wrapper<Chunk>> World::getChunkWithMesh(int x, int y){
    auto chunkOpt = this->getChunk(x, y);
    if(!chunkOpt) return std::nullopt;

    Chunk& chunk = chunkOpt.value();

    if(!chunk.meshGenerated && !chunk.meshGenerating){
        std::thread t1(generateChunkMeshThread, chunk);

        return std::nullopt;
    }

    if(!chunk.buffersLoaded && chunk.meshGenerated){
        if(!chunk.buffersAsigned){
            chunk.solidBuffer = GLBuffer();
            chunk.solidBackBuffer = GLBuffer();

            chunk.transparentBuffer = GLBuffer();
            chunk.transparentBackBuffer = GLBuffer();
            chunk.buffersAsigned = 1;
        }
        //printf("Loading meshes %2i:%2i (%i)...\n",x,z,chunk->buffersLoaded);

        //printf("Vertices:%i Indices:%i\n", chunk->solidMesh->vertices_count, chunk->solidMesh->indices_count);
        chunk.solidBackBuffer.loadMesh(chunk.solidMesh.value());
        chunk.transparentBackBuffer.loadMesh(chunk.transparentMesh.value());

        chunk.solidMesh.reset();
        chunk.transparentMesh.reset();
        
        GLBuffer tempSolid = chunk.solidBuffer;
        GLBuffer tempTransparent = chunk.transparentBuffer;

        chunk.solidBuffer = chunk.solidBackBuffer;
        chunk.transparentBuffer =  chunk.transparentBackBuffer;

        chunk.solidBackBuffer = tempSolid;
        chunk.transparentBackBuffer = tempTransparent;

        chunk.buffersLoaded = 1;
        chunk.isDrawn = 1;
    }

   return chunk;
}

CollisionCheckResult World::checkForPointCollision(float x, float y, float z, int includeRectangularColliderLess){
    CollisionCheckResult result = {std::nullopt, false, 0,0,0};
    int range = 3;

    float blockWidth = 1;

    for(int i = -range;i <= range;i++){
        for(int j = -range;j <= range;j++){
            for(int g = -range;g <= range;g++){
                int cx = x + i;
                int cy = y + j;
                int cz = z + g;

                auto blockOpt = this->getBlock(cx, cy, cz);
                if(blockOpt){
                    const Block& blocki = blockOpt.value();

                    BlockType block = predefinedBlocks[blocki.type];
                    if(block.colliders.size() == 0 && !includeRectangularColliderLess) continue;

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
    CollisionCheckResult result = {std::nullopt, false, 0,0,0};
    int range = 3;

    for(int i = -range;i <= range;i++){
        for(int j = -range;j <= range;j++){
            for(int g = -range;g <= range;g++){
                int cx = (int)floor(x + i);
                int cy = (int)floor(y + j);
                int cz = (int)floor(z + g);

                auto blockOpt = this->getBlock(cx, cy, cz);
                if(blockOpt){
                    const Block& blocki = blockOpt.value();

                    BlockType block = predefinedBlocks[blocki.type];
                    if(block.colliders.size() == 0) continue;

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

std::optional<std::reference_wrapper<const Block>> World::getBlock(int x, int y, int z){
    if(y < 0 || y > DEFAULT_CHUNK_HEIGHT) return std::nullopt;
    
    int chunkX = floor((double)x / (double)DEFAULT_CHUNK_SIZE);
    int chunkZ = floor((double)z / (double)DEFAULT_CHUNK_SIZE);

    int ix = abs(x - chunkX * DEFAULT_CHUNK_SIZE);
    int iz = abs(z - chunkZ * DEFAULT_CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %i(%i)x%ix%i(%i)\n", chunkX, chunkZ, ix,y,iz);

    auto chunkOpt = this->getChunk(chunkX, chunkZ);
    if(!chunkOpt) return this->generateChunk(chunkX, chunkZ).getBlock(ix,y,iz);

    return chunkOpt.value().get().getBlock(ix, y, iz);
}

std::optional<std::reference_wrapper<Chunk>> World::getChunkFromBlockPosition(int x, int z){
    int cx,cz;
    if(x >= 0) cx = x / DEFAULT_CHUNK_SIZE;
    else cx = x / DEFAULT_CHUNK_SIZE - 1;
    if(z >= 0) cz = z / DEFAULT_CHUNK_SIZE;
    else cz = z / DEFAULT_CHUNK_SIZE - 1;

    return this->getChunk(cx, cz);
}

int World::setBlock(int x, int y, int z, Block index){
    if(y < 0 || y > DEFAULT_CHUNK_HEIGHT) return INVALID_COORDINATES;
    
    int chunkX = floor((double)x / (double)DEFAULT_CHUNK_SIZE);
    int chunkZ = floor((double)z / (double)DEFAULT_CHUNK_SIZE);

    int ix = abs(x - chunkX * DEFAULT_CHUNK_SIZE);
    int iz = abs(z - chunkZ * DEFAULT_CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %ix%ix%i\n", chunkX, chunkZ, ix,y,iz);

    //Block* i = getWorldBlock(world, ix, y, iz);

    auto chunkOpt = this->getChunk(chunkX, chunkZ);
    if(!chunkOpt) this->generateChunk(chunkX, chunkZ).setBlock(ix,y,iz,index);

    chunkOpt.value().get().setBlock(ix, y, iz, index);

    return OK;
}
