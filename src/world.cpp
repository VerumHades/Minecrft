#include <world.hpp>

std::shared_mutex chunkGenLock;

Chunk* World::generateAndGetChunk(int x, int y, int z){
    const glm::vec3 key = glm::vec3(x,y,z);

    {
        std::shared_lock lock(chunkGenLock);
        auto it = this->chunks.find(key);
        if (it != this->chunks.end()) {
            return &it->second;
        } 
    }
    
    std::unique_lock lock(chunkGenLock);
    auto [iter, success] = this->chunks.emplace(key, Chunk(*this, key));
    generateTerrainChunk(iter->second,x,z);
    return &iter->second;
}

Chunk* World::getChunk(int x, int y, int z){
    const glm::vec3 key = glm::vec3(x,y,z);
    
    std::shared_lock lock(chunkGenLock);

    auto it = this->chunks.find(key);
    if (it != this->chunks.end()) {
        return &it->second;
    }

    return nullptr;
}

std::size_t Vec3Hash::operator()(const glm::vec3& v) const noexcept {
    std::size_t h1 = std::hash<float>{}(v.x);
    std::size_t h2 = std::hash<float>{}(v.y);
    std::size_t h3 = std::hash<float>{}(v.z);
    // Combine the hash values
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

bool Vec3Equal::operator()(const glm::vec3& lhs, const glm::vec3& rhs) const noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

static int maxThreads = 6;
//const auto maxThreads = 1;
static int threadsTotal = 0;
void generateChunkMeshThread(Chunk* chunk){
    //auto start = std::chrono::high_resolution_clock::now();

    //std::cout << "Generating mesh: " << &chunk <<  std::endl;
        
    chunk->generateMeshes();

    // End time point
    //auto end = std::chrono::high_resolution_clock::now();

    //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //std::cout << "Execution time: " << duration << " microseconds" << std::endl;

    chunk->meshGenerating = false;
    chunk->meshGenerated = true;
    threadsTotal--;
}

Chunk* World::getChunkWithMesh(int x, int y, int z){
    Chunk* chunk = this->getChunk(x, y, z);
    if(!chunk) return nullptr;

    if(!chunk->meshGenerated && !chunk->meshGenerating && threadsTotal < maxThreads){
        threadsTotal++;
        std::thread t1(generateChunkMeshThread, chunk);
        t1.detach();


        chunk->meshGenerating = true;
        //generateChunkMeshThread(chunk);
        if(chunk->isDrawn) return chunk;
        return nullptr;
    } 

    if(!chunk->buffersLoaded && !chunk->buffersInQue && chunk->meshGenerated){
        chunk->buffersInQue = true;
        this->bufferLoadQue.push(chunk->getWorldPosition());
        //printf("Vertices:%i Indices:%i\n", chunk->solidMesh->vertices_count, chunk->solidMesh->indices_count)
        updated = true;
        if(chunk->isDrawn) return chunk;
    }

   return chunk;
}

void World::updateBuffers(){
    if(this->bufferLoadQue.empty()) return;
    glm::vec3 currentPos = this->bufferLoadQue.front();

    Chunk* chunk = this->getChunk((int) currentPos.x,(int) currentPos.y, (int) currentPos.z);

    if(!chunk->solidBuffer){
        chunk->solidBuffer = std::make_unique<GLDoubleBuffer>();
    }

    //std::cout << "Loading mesh: " << chunk->getWorldPosition().x << " " << chunk->getWorldPosition().y << " " << chunk->getWorldPosition().z << std::endl;

    //printf("Loading meshes %2i:%2i (%i)...\n",x,z,chunk->buffersLoaded);
    if (chunk->solidBuffer && chunk->solidMesh) {
        GLDoubleBuffer& dbuffer = *chunk->solidBuffer;
        
        dbuffer.getBackBuffer().loadMesh(*chunk->solidMesh);
        dbuffer.swap();
        chunk->solidMesh = nullptr;

        chunk->buffersLoaded = true;
        chunk->buffersInQue = false;
        chunk->isDrawn = true;

        this->bufferLoadQue.pop();
    }
    else{
        std::cerr << "Failed to load chunk buffer!" << std::endl;
    }
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

void World::drawChunks(Camera& camera, ShaderProgram& program, int renderDistance){
    int camWorldX = (int) camera.getPosition().x / CHUNK_SIZE;
    int camWorldY = (int) camera.getPosition().y / CHUNK_SIZE;
    int camWorldZ = (int) camera.getPosition().z / CHUNK_SIZE;

    for(int x = -renderDistance; x <= renderDistance; x++){
        for(int z = -renderDistance; z <= renderDistance; z++){
            int chunkX = x + camWorldX;
            int chunkY = 0;
            int chunkZ = z + camWorldZ;

            //std::cout << "Drawing chunk: " << chunkX << " " << chunkZ << " " << camera.getPosition().x << " " << camera.getPosition().z <<std::endl;

            Chunk* meshlessChunk = this->getChunk(chunkX, chunkY, chunkZ);
            if(!meshlessChunk) continue;

            //std::cout << "Got chunk!" << std::endl;
            if(!camera.isVisible(*meshlessChunk) && !(glm::length(glm::vec2(x,z)) <= 2)) continue;
            Chunk* chunk = this->getChunkWithMesh(chunkX, chunkY, chunkZ);
            
            //std::cout << "Got meshed chunk!" << std::endl;
            if(!chunk) continue;
            if(!chunk->isDrawn) continue;

            camera.setModelPosition( {
                (float) chunkX * CHUNK_SIZE,
                (float) chunkY * CHUNK_SIZE,
                (float) chunkZ * CHUNK_SIZE
            });
            program.updateUniform("modelMatrix");

            if(chunk->solidBuffer) chunk->solidBuffer->getBuffer().draw();
        }
    }
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
