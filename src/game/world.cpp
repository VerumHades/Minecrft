#include <game/world.hpp>

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

    std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>(*this, key);
    
    generator.generateTerrainChunk(*chunk,x,y,z, lod);

    std::unique_lock lock(chunkGenLock);
    this->chunks.emplace(key, std::move(chunk));
    this->stream->save(*chunks[key]);

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
    chunks[position] = std::make_unique<Chunk>(*this, position);
    stream->load(chunks[position].get());
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

void World::addToChunkMeshLoadingQueue(glm::ivec3 position, std::unique_ptr<Mesh> mesh){
    std::lock_guard<std::mutex> lock(meshLoadingMutex);
    meshLoadingQueue.push({position,std::move(mesh)});
}
void World::loadMeshFromQueue(ChunkMeshRegistry&  buffer){
    std::lock_guard<std::mutex> lock(meshLoadingMutex);
    if(meshLoadingQueue.empty()) return;
    auto& [position,mesh] = meshLoadingQueue.front();

    bool loaded = false;
    
    if(buffer.isChunkLoaded(position)) loaded = buffer.swapChunkMesh(*mesh, position);
    else loaded = buffer.addChunkMesh(*mesh, position);

    if(loaded) meshLoadingQueue.pop();
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

WorldStream::WorldStream(std::string filepath){
    bool newlyCreated = false;
    
    if(!std::filesystem::exists(filepath)){
        std::ofstream file(filepath, std::ios::binary);
        if(!file.is_open()){
            std::cout << "World stream opening failed, cannot create file: " << filepath << std::endl;
            std::terminate();
        }
        file.close();
        newlyCreated = true;
        std::cout << "Created missing world file." << std::endl;
    }

    file_stream = std::fstream(filepath, std::ios::in | std::ios::out | std::ios::binary);

    if(!file_stream.is_open()){
        std::cout << "Failed to open world file!" << std::endl;
        std::terminate();
    }

    if(newlyCreated){
        std::strcpy(header.name,"World");

        std::random_device rd;
        header.seed = rd(); 
        
        header.chunk_table_start = sizeof(Header);
        header.chunk_data_start = 20 * 1000; // Reserve ahead for about 1000 chunks
        header.chunk_data_end = 20 * 1000;
        header.chunk_table_size = 0;


        saveHeader();
        saveTable();
    } // Create the header table if its a new save file
    else{
        loadHeader();
        loadTable();
    }
}

void WorldStream::saveHeader(){
    file_stream.seekp(0, std::ios::beg); // Move cursor to the file start
    
    bitworks::saveValue(file_stream, header);
}

void WorldStream::loadHeader(){
    file_stream.seekg(0, std::ios::beg); // Move cursor to the file start
    
    header = bitworks::readValue<Header>(file_stream);

    //std::cout << "Table start: " << header.chunk_table_start << " Chunk data start: " << header.chunk_data_start << std::endl;
}

void WorldStream::loadTable(){
    file_stream.seekg(header.chunk_table_start, std::ios::beg); // Move cursor to the tables start
    chunkTable.clear();

    ByteArray tableData;
    if(!tableData.read(file_stream)){
        std::cout << "World file table corrupted, repairing" << std::endl;
        saveTable();
        return;
    }

    size_t size = tableData.read<size_t>();
    //std::cout << "Chunks total: " << size << std::endl;
    for(int i = 0;i < size;i++){
        glm::vec3 position = {
            tableData.read<float>(),
            tableData.read<float>(),
            tableData.read<float>()
        };
        size_t start = tableData.read<size_t>();
        if(start > header.chunk_data_end) {
            std::cout << "Corrupted chunk data, disposing." << std::endl;
            continue; 
        }
        chunkTable[position] = start;
    }
}

ByteArray WorldStream::serializeTableData(){
    ByteArray tableData;
    tableData.append<size_t>(chunkTable.size());

    for(auto& [position, start]: chunkTable){
        tableData.append<float>(position.x);
        tableData.append<float>(position.y);
        tableData.append<float>(position.z);
        tableData.append<size_t>(start);
    }

    return tableData;
}

void WorldStream::saveTable(){
    ByteArray tableData = serializeTableData();
    while(header.chunk_table_start + tableData.getFullSize() >= header.chunk_data_start){
        size_t movedSize = moveChunk(header.chunk_data_start, header.chunk_data_end); // Move the first chunk to the end

        //std::cout << movedSize << " > " << tableData.getFullSize() << std::endl;
        header.chunk_data_start += movedSize;
        header.chunk_data_end += movedSize;
    }
    tableData = serializeTableData();

    file_stream.seekp(header.chunk_table_start, std::ios::beg);
    tableData.write(file_stream);
    saveHeader();
}

size_t WorldStream::moveChunk(size_t from, size_t to){
    file_stream.seekg(from, std::ios::beg); // Move to the position
    
    ByteArray fromData;
    fromData.read(file_stream);

    glm::vec3 position = {
        fromData.read<float>(),
        fromData.read<float>(),
        fromData.read<float>()
    };

    file_stream.seekp(to, std::ios::beg); 
    fromData.write(file_stream);

    chunkTable[position] = to;

    return fromData.getFullSize();
}

/*
    Saves a chunk mask in this format:

    int type
    size_t compressed_24bit_count, data...
    size_t compressed_24bit_count_rotated, data...
*/
static inline void saveMask(ByteArray& out, DynamicChunkMask& mask, int type){
    out.append<int>(type);
    out.append(mask.getSegments().compress().data);
}

/*
    Saved the chunk:
    float x,y,z
    size_t layerCount
    saved masks using the above function..
*/
bool WorldStream::save(Chunk& chunk){
    if(hasChunkAt(chunk.getWorldPosition())) return false;
    ByteArray out;

    out.append(chunk.getWorldPosition().x);
    out.append(chunk.getWorldPosition().y);
    out.append(chunk.getWorldPosition().z);
    
    // Get the 64 bit masks if they exist otherwise fail
    if(!chunk.isMainGroupOfSize(64)) return false;
    auto& group = chunk.getMainGroup();

    size_t layer_count = group->getMasks().size();
    out.append(layer_count);

    saveMask(out, group->getSolidMask(), -1);
    for(auto& [key, mask]: group->getMasks()) saveMask(out, mask, static_cast<int>(mask.getBlock().type));

    file_stream.seekp(header.chunk_data_end, std::ios::beg);
    out.write(file_stream);

    chunkTable[chunk.getWorldPosition()] = header.chunk_data_end;
    header.chunk_data_end += out.getFullSize();

    saveHeader();
    saveTable();
    return true;
}

void WorldStream::load(Chunk* chunk){
    if(!hasChunkAt(chunk->getWorldPosition())){
        return;
    }

    //std::cout << "Loadeding: " << chunk->getWorldPosition().x << " " << chunk->getWorldPosition().y << " " << chunk->getWorldPosition().z << std::endl;
    //std::cout << "Located at: " << chunkTable[chunk->getWorldPosition()] << std::endl;
    file_stream.seekg(chunkTable[chunk->getWorldPosition()], std::ios::beg);
    ByteArray source;
    
    if(!source.read(file_stream)){
        std::cout << "Corrupted chunk:" << chunk->getWorldPosition().x << " " << chunk->getWorldPosition().y << " " << chunk->getWorldPosition().z << std::endl;
        return;
    }
    
    //std::cout << "Loaded: " << chunk->getWorldPosition().x << " " << chunk->getWorldPosition().y << " " << chunk->getWorldPosition().z << std::endl;

    //std::cout << source << std::endl;

    glm::vec3 position = {
        source.read<float>(),
        source.read<float>(),
        source.read<float>()
    };
    size_t layerCount = source.read<size_t>();

    //std::cout << "Found: " << position.x << " " << position.y << " " << position.z << std::endl;
    //std::cout << "With total layers: " << layerCount << std::endl;

    source.read<int>();

    std::unique_ptr<DynamicChunkContents> outputDataGroup = std::make_unique<DynamicChunkContents>(64);

    DynamicChunkMask solidMask = DynamicChunkMask(64, {source.vread<compressed_24bit>(), 64 * 64});
    outputDataGroup->setSolidMask(solidMask);

    for(int layerIndex = 0; layerIndex < layerCount; layerIndex++){
        int type = source.read<int>();
        DynamicChunkMask mask =  DynamicChunkMask(64, {source.vread<compressed_24bit>(), 64 * 64});
        outputDataGroup->setMask(static_cast<BlockTypes>(type),mask);
    }

    chunk->setMainGroup(std::move(outputDataGroup));
    
    //std::cout << "Loaded: " << position.x << " " << position.y << " " << position.z << std::endl;
}

bool WorldStream::hasChunkAt(glm::vec3 position){
    return chunkTable.count(position) != 0;
}

WorldStream::~WorldStream(){
    saveHeader();
    saveTable();
}