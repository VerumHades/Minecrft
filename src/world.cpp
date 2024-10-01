#include <world.hpp>

std::shared_mutex chunkGenLock;

void World::generateChunk(int x, int y, int z){
    const glm::vec3 key = glm::vec3(x,y,z);

    {
        std::shared_lock lock(chunkGenLock);
        auto it = this->chunks.find(key);
        if (it != this->chunks.end()) return;
    }

    std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>(*this, key);
    generateTerrainChunk(*chunk,x,y,z);

    std::unique_lock lock(chunkGenLock);
    this->chunks.emplace(key, std::move(chunk));
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

static int maxThreads = 16;
//const auto maxThreads = 1;
static int threadsTotal = 0;
void generateChunkMeshThread(Chunk* chunk){
    //auto start = std::chrono::high_resolution_clock::now();
    //std::cout << "Generating mesh: " << &chunk <<  std::endl;
        
    chunk->generateMeshes();

    //End time point
    //auto end = std::chrono::high_resolution_clock::now();

    //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //std::cout << "Execution time: " << duration << " microseconds" << std::endl;

    chunk->meshGenerating = false;
    chunk->meshGenerated = true;
    threadsTotal--;
}

void World::generateChunkMesh(int x, int y, int z, MultiChunkBuffer& buffer){
    Chunk* chunk = this->getChunk(x, y, z);
    if(!chunk) return;

    if(
        !getChunk(x - 1,y,z) ||
        !getChunk(x,y - 1,z) ||
        !getChunk(x,y,z - 1) 
    ) return;

    if(!chunk->meshGenerated && !chunk->meshGenerating && threadsTotal < maxThreads){
        threadsTotal++;
        std::thread t1(generateChunkMeshThread, chunk);
        t1.detach();


        chunk->meshGenerating = true;
        //generateChunkMeshThread(chunk);
        return;
    } 

    if(!chunk->buffersLoaded && chunk->meshGenerated){
        if(buffer.isChunkLoaded(chunk->getWorldPosition())) buffer.swapChunkMesh(*chunk->solidMesh, chunk->getWorldPosition());
        else buffer.addChunkMesh(*chunk->solidMesh, chunk->getWorldPosition());
        chunk->buffersLoaded = true;
        chunk->solidMesh = nullptr;
        return;
    }
    /*if(!chunk->buffersLoaded && !chunk->buffersInQue && chunk->meshGenerated){
        chunk->buffersInQue = true;
        this->bufferLoadQue.push(chunk->getWorldPosition());
        //printf("Vertices:%i Indices:%i\n", chunk->solidMesh->vertices_count, chunk->solidMesh->indices_count)
        updated = true;
        if(chunk->isDrawn) return chunk;
    }*/
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


template <typename T>
static inline void saveValue(std::ofstream &file, T value){
    file.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
static inline T readValue(std::ifstream &file) {
    T value;
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
}

static inline void saveBitArray3D(std::ofstream &file, BitArray3D& array){
    std::vector<compressed_24bit> compressedMask        = bitworks::compressBitArray3D(array);

    saveValue<size_t>(file, compressedMask.size());
    for(auto& value: compressedMask) saveValue(file,value);
}

static inline BitArray3D readBitArray3D(std::ifstream &file){
    size_t count = readValue<size_t>(file);

    std::cout << "Reading compressed array: " << count << std::endl;
    std::vector<compressed_24bit> compressed = {};
    for(int i = 0;i < count;i++) compressed.push_back(readValue<compressed_24bit>(file));

    return bitworks::decompressBitArray3D(compressed);
}
/*
    Saves a chunk mask in this format:

    int type
    size_t compressed_24bit_count, data...
    size_t compressed_24bit_count_rotated, data...
*/
static inline void saveMask(std::ofstream &file, ChunkMask& mask, int type){
    saveValue<int>(file, type);
    saveBitArray3D(file, mask.segments);
    saveBitArray3D(file, mask.segmentsRotated);
}

/*
    Saved the chunk:
    size_t layerCount
    float x,y,z
    saved masks using the above function..
*/
void World::saveChunk(std::ofstream &file, Chunk& chunk){  
    size_t layer_count = chunk.getMasks().size() + 1; // Accomodate solid mask  
    saveValue(file, layer_count);

    saveValue(file, chunk.getWorldPosition().x);
    saveValue(file, chunk.getWorldPosition().y);
    saveValue(file, chunk.getWorldPosition().z);

    saveMask(file, chunk.getSolidMask(), -1);
    for(auto& [key, mask]: chunk.getMasks()) saveMask(file, mask, static_cast<int>(mask.block.type));
}

void World::save(std::string filepath){
    std::ofstream file(filepath, std::ios::binary);
    if(!file.is_open()){
        std::cout << "World save failed, cannot open file: " << filepath << std::endl;
    }

    saveValue(file, chunks.size());
    for(auto& [key,chunk]: chunks) saveChunk(file, *chunk);
}

void World::load(std::string filepath){
    std::unique_lock lock(chunkGenLock);

    std::ifstream file(filepath, std::ios::binary);
    if(!file.is_open()){
        std::cout << "World save failed, cannot open file: " << filepath << std::endl;
    }   

    size_t chunkCount = readValue<size_t>(file);
    std::cout << "Chunk count: " << chunkCount << std::endl;

    for(size_t chunkIndex = 0;chunkIndex < chunkCount;chunkIndex++){
        size_t layerCount = readValue<size_t>(file);

        glm::vec3 position = {
            readValue<float>(file),
            readValue<float>(file),
            readValue<float>(file)
        };

        std::cout << "Loading chunk: " << position.x << "," << position.y << "," << position.z << std::endl;
        
        chunks[position] = std::make_unique<Chunk>(*this, position);
        Chunk* chunk = chunks.at(position).get();

        for(int layerIndex = 0; layerIndex < layerCount; layerIndex++){
            int type = readValue<int>(file);
            BitArray3D normal = readBitArray3D(file);
            BitArray3D rotated = readBitArray3D(file);
            
            if(layerIndex == 0){
                chunk->getSolidMask().segments = normal;
                chunk->getSolidMask().segmentsRotated = rotated;
                continue;
            }

            std::cout << "Loading chunk layers: " << layerIndex << " " << getBlockTypeName(static_cast<BlockTypes>(type)) << std::endl;

            ChunkMask mask;
            mask.segments = normal;
            mask.segmentsRotated = rotated;

            chunk->getMasks()[static_cast<BlockTypes>(type)] = mask;
        }
    }
}