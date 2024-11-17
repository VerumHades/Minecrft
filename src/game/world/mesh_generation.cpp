#include <game/world/mesh_generation.hpp>


void ChunkMeshGenerator::addToChunkMeshLoadingQueue(glm::ivec3 position, std::unique_ptr<Mesh> mesh){
    std::lock_guard<std::mutex> lock(meshLoadingMutex);
    meshLoadingQueue.push({position,std::move(mesh)});
}
void ChunkMeshGenerator::loadMeshFromQueue(ChunkMeshRegistry&  buffer){
    std::lock_guard<std::mutex> lock(meshLoadingMutex);
    if(meshLoadingQueue.empty()) return;
    auto& [position,mesh] = meshLoadingQueue.front();

    bool loaded = false;
    
    if(buffer.isChunkLoaded(position)) loaded = buffer.updateMesh(mesh.get(), position);
    else loaded = buffer.addMesh(mesh.get(), position);

    if(loaded){
        meshLoadingQueue.pop();
    }
}

void ChunkMeshGenerator::syncGenerateAsyncUploadMesh(Chunk* chunk){
    auto world_position = chunk->getWorldPosition();
    auto solid_mesh = generateChunkMesh(world_position, chunk);

    addToChunkMeshLoadingQueue(world_position, std::move(solid_mesh));
}

void ChunkMeshGenerator::asyncGenerateAsyncUploadMesh(Chunk* chunk, ThreadPool& pool){
    bool success = pool.deploy([this, chunk](){
        syncGenerateAsyncUploadMesh(chunk);
    });
}

void ChunkMeshGenerator::syncGenerateSyncUploadMesh(Chunk* chunk, ChunkMeshRegistry& buffer){
    auto world_position = chunk->getWorldPosition();
    auto solid_mesh = generateChunkMesh(world_position, chunk);

    if(buffer.isChunkLoaded(world_position)) buffer.updateMesh(solid_mesh.get(), world_position);
    else buffer.addMesh(solid_mesh.get(), world_position);
}

/*
    Generate greedy meshed faces from a plane of bits
*/
std::vector<ChunkMeshGenerator::Face> ChunkMeshGenerator::greedyMeshPlane(std::array<uint64_t, 64> rows, int size){
    std::vector<Face> out = {};

    /*
        Moves bits to from the right to the left

        if size = 4
        00001111
        then it gets converted to this for meshing (because count_leading_zeros starts from the left )
        11110000
    */
    int size_to_real_size_adjustment = 64 - size;
    for(int i = 0;i < size;i++) rows[i] <<= size_to_real_size_adjustment;

    int currentRow = 0;
    
    while(currentRow < size){
        uint64_t row = rows[currentRow];
        /*
            0b00001101

            'start' is 4
        */    
        uint8_t start = std::min(count_leading_zeros(row), (uint8_t) size); // Find the first
        if(start == size){
            currentRow++;
            continue;
        }
        /*
            2. 0b11010000 shift by 4
            1. 0b00101111 negate

            'width' is 2
        */    
        row <<= start; // Shift so the faces start
        uint8_t width = std::min(count_leading_zeros(~row), static_cast<uint8_t>(size - start)); // Calculate width (negated counts '1') 
        row >>= start; // Return to original position

        /*
                Create a mask of all ones
                1. 0b11111111
                2. 0b00000011 shift beyond the face (start + width)
                3. 0b11111100 negate

                    0b11111100 & 0b00001101

                4. 0b00001100 AND with the row to create the faces mask
        */
        uint64_t mask = ~0ULL;

        //  Shifting by 64 is undefined behaviour for some reason ¯\_(ツ)_/¯
        if((start + width) != size) mask = ~(mask >> (start + width));
        mask &= row;

        int height = 0; 
        while(currentRow + height < size && (mask & rows[currentRow + height]) == mask){
            rows[currentRow + height]  &= ~mask; // Remove this face part from the rows
            height++;
        }

        out.push_back({ // Add the face to the face list
            start, currentRow,
            width, height
        });

        //return out;
    }

    return out;
}
/*
    TODO: 

        Haven't broken anything yet


*/

static inline std::unordered_set<BlockType> mergeMaskKeys(const std::unordered_set<BlockType>& a, const std::unordered_set<BlockType>& b){
    std::unordered_set<BlockType> out;
    for(auto& key: a) out.emplace(key);
    for(auto& key: b) out.emplace(key);
    return out;
}

enum FaceDirection{
    X,Y,Z
};

static inline void processFaces(std::vector<ChunkMeshGenerator::Face> faces, FaceDirection direction, bool forward, BlockDefinition& type, Mesh* mesh, int worldX, int worldY, int worldZ, int layer, float scale){
    std::array<glm::vec3, 4> vertices;
    int texture = 0;
    int normal;
    bool clockwise;

    float occlusion[4] = {0,0,0,0};
    
    glm::vec3 worldOffset = {worldX,worldY,worldZ};

    for(auto& face: faces){
        switch(direction){
            case X:
                vertices = {
                    glm::vec3(layer + 1, face.y + face.height, face.x             ) * scale + worldOffset,
                    glm::vec3(layer + 1, face.y + face.height, face.x + face.width) * scale + worldOffset,
                    glm::vec3(layer + 1, face.y              , face.x + face.width) * scale + worldOffset,
                    glm::vec3(layer + 1, face.y              , face.x             ) * scale + worldOffset
                };
                texture = type.repeatTexture ? type.textures[0] : type.textures[4 + forward];
                normal = forward ? 3 : 2;
                clockwise = forward;
                break;
            case Y:
                vertices = {
                    glm::vec3(face.x             , layer + 1, face.y              ) * scale + worldOffset,
                    glm::vec3(face.x + face.width, layer + 1, face.y              ) * scale + worldOffset,
                    glm::vec3(face.x + face.width, layer + 1, face.y + face.height) * scale + worldOffset,
                    glm::vec3(face.x             , layer + 1, face.y + face.height) * scale + worldOffset
                };

                texture = type.repeatTexture ? type.textures[0] : type.textures[!forward];
                normal = forward ? 1 : 0;
                clockwise = !forward;
                break;
            case Z:
                vertices = {
                    glm::vec3(face.x              , face.y + face.height, layer + 1) * scale + worldOffset,
                    glm::vec3(face.x + face.width , face.y + face.height, layer + 1) * scale + worldOffset,
                    glm::vec3(face.x + face.width , face.y              , layer + 1) * scale + worldOffset,
                    glm::vec3(face.x              , face.y              , layer + 1) * scale + worldOffset
                };

                texture = type.repeatTexture ? type.textures[0] : type.textures[2 + forward];
                normal = forward ? 5 : 4;
                clockwise = !forward;
                break;
        }

        mesh->addQuadFaceGreedy(
            vertices.data(),
            normal,
            occlusion,
            static_cast<float>(texture),
            clockwise,
            face.width,
            face.height
        );
    }
}   

std::unique_ptr<Mesh> ChunkMeshGenerator::generateChunkMesh(glm::ivec3 worldPosition, Chunk* group){
    auto solidMesh = std::make_unique<Mesh>();
    if(!group) return solidMesh;
    //this->solidMesh->setVertexFormat(VertexFormat({3,1,2,1,1}));  // Unused
    float worldX = worldPosition.x * CHUNK_SIZE;
    float worldY = worldPosition.y * CHUNK_SIZE;
    float worldZ = worldPosition.z * CHUNK_SIZE;

    int size = CHUNK_SIZE;
    float scale = 1.0;

    //std::cout << "Scale: " << scale << " size: " << size << std::endl;
    /*
        Mesh chunk faces
    */

    auto* solidRotated = group->getSolidField().getTransposed(&cache);

    for(int z = 0; z < size - 1;z++){
        BlockBitPlanes<64> planesXforward = {0};
        BlockBitPlanes<64> planesXbackward = {0};

        BlockBitPlanes<64> planesYforward = {0};
        BlockBitPlanes<64> planesYbackward = {0};

        BlockBitPlanes<64> planesZforward = {0};
        BlockBitPlanes<64> planesZbackward = {0};
        
        for(int y = 0;y < size;y++){
            for(auto& [type,block,field]: group->getLayers()){
                auto* rotatedField = field.getTransposed(&cache);

                uint64_t allFacesX = (rotatedField->getRow(z,y) | rotatedField->getRow(z + 1,y)) & (solidRotated->getRow(z,y) ^ solidRotated->getRow(z + 1,y));
                planesXforward[ (size_t) type][y] = solidRotated->getRow(z,y)     & allFacesX;
                planesXbackward[(size_t) type][y] = solidRotated->getRow(z + 1,y) & allFacesX;

                uint64_t allFacesY = (field.getRow(y,z) | field.getRow(y,z + 1)) & (group->getSolidField().getRow(y,z) ^ group->getSolidField().getRow(y,z + 1));
                planesYforward[ (size_t) type][y] = group->getSolidField().getRow(y,z)     & allFacesY;
                planesYbackward[(size_t) type][y] = group->getSolidField().getRow(y,z + 1) & allFacesY;

                uint64_t allFacesZ = (field.getRow(z,y) | field.getRow(z + 1,y)) & (group->getSolidField().getRow(z,y) ^ group->getSolidField().getRow(z + 1,y));
                planesZforward[ (size_t) type][y] = group->getSolidField().getRow(z,y)     & allFacesZ;
                planesZbackward[(size_t) type][y] = group->getSolidField().getRow(z + 1,y) & allFacesZ;
            }
        }

        for(auto& type: group->getPresentTypes()){
            BlockDefinition definition = predefinedBlocks[type];
            if(!definition.solid || definition.billboard) continue;
            //std::cout << "Solving plane: " << i << std::endl;
            //for(int j = 0;j < 64;j++) std::cout << std::bitset<64>(planes[i][j]) << std::endl;

            processFaces(greedyMeshPlane(planesXforward [static_cast<size_t>(type)], size), X, true , definition, solidMesh.get(), worldX, worldY, worldZ, z, scale);
            processFaces(greedyMeshPlane(planesXbackward[static_cast<size_t>(type)], size), X, false, definition, solidMesh.get(), worldX, worldY, worldZ, z, scale);

            processFaces(greedyMeshPlane(planesYforward [static_cast<size_t>(type)], size), Y, true , definition, solidMesh.get(), worldX, worldY, worldZ, z, scale);
            processFaces(greedyMeshPlane(planesYbackward[static_cast<size_t>(type)], size), Y, false, definition, solidMesh.get(), worldX, worldY, worldZ, z, scale);

            processFaces(greedyMeshPlane(planesZforward [static_cast<size_t>(type)], size), Z, true , definition, solidMesh.get(), worldX, worldY, worldZ, z, scale);
            processFaces(greedyMeshPlane(planesZbackward[static_cast<size_t>(type)], size), Z, false, definition, solidMesh.get(), worldX, worldY, worldZ, z, scale);
        }
    }
    
    /*
        Mesh bilboards
    */

    for(auto& [type,block,field]: group->getLayers()){
        BlockDefinition definition = predefinedBlocks[type];
        if(!definition.billboard) continue;

        for(int x = 0;x < size;x++) for(int y = 0;y < size;y++) for(int z = 0;z < size;z++){
            if(!field.get(x,y,z)) continue;

            int normal = 0;

            float occlusion[4] = {0,0,0,0};
            glm::vec3 worldOffset = {worldX,worldY,worldZ};
            glm::vec3 position = {z,y,x};
            
            std::array<glm::vec3, 4> vertices1 = {
                (glm::vec3(0,1,0) + position) * scale + worldOffset,
                (glm::vec3(1,1,1) + position) * scale + worldOffset,
                (glm::vec3(1,0,1) + position) * scale + worldOffset,
                (glm::vec3(0,0,0) + position) * scale + worldOffset
            };

            std::array<glm::vec3, 4> vertices2 = {
                (glm::vec3(0,1,1) + position) * scale + worldOffset,
                (glm::vec3(1,1,0) + position) * scale + worldOffset,
                (glm::vec3(1,0,0) + position) * scale + worldOffset,
                (glm::vec3(0,0,1) + position) * scale + worldOffset
            };
            
            solidMesh->addQuadFaceGreedy(
                vertices1.data(),
                normal,
                occlusion,
                static_cast<float>(definition.textures[0]), // Texture is the first one
                true,
                1,1
            );
            solidMesh->addQuadFaceGreedy(
                vertices1.data(),
                normal,
                occlusion,
                static_cast<float>(definition.textures[0]), // Texture is the first one
                false,
                1,1
            );

            solidMesh->addQuadFaceGreedy(
                vertices2.data(),
                normal,
                occlusion,
                static_cast<float>(definition.textures[0]), // Texture is the first one
                true,
                1,1
            );
            solidMesh->addQuadFaceGreedy(
                vertices2.data(),
                normal,
                occlusion,
                static_cast<float>(definition.textures[0]), // Texture is the first one
                false,
                1,1
            );
        }
    }

    return solidMesh;
    /*
        Mesh cross chunk faces
    
    BlockBitPlanes<64> planesXforward = {0};
    BlockBitPlanes<64> planesXbackward = {0};

    BlockBitPlanes<64> planesYforward = {0};
    BlockBitPlanes<64> planesYbackward = {0};

    BlockBitPlanes<64> planesZforward = {0};
    BlockBitPlanes<64> planesZbackward = {0};
    

    Chunk* nextX = world.getChunk(worldPosition.x - 1,worldPosition.y,worldPosition.z);
    Chunk* nextY = world.getChunk(worldPosition.x,worldPosition.y - 1,worldPosition.z);
    Chunk* nextZ = world.getChunk(worldPosition.x,worldPosition.y,worldPosition.z - 1);

    if(!nextX || !nextY || !nextZ){
        std::cerr << "Mesh generating when chunks are missing?" << std::endl;
        return solidMesh;
    }

    if(!nextX->isMainGroupOfSize(size) || !nextY->isMainGroupOfSize(size) || !nextZ->isMainGroupOfSize(size)){
        //std::cout << "Mesh generating when wrong sizes?" << std::endl;
        return solidMesh;
    }

    DynamicChunkMask& nextXSolid = nextX->getMainGroup()->getSolidMask();
    DynamicChunkMask& nextYSolid = nextY->getMainGroup()->getSolidMask();
    DynamicChunkMask& nextZSolid = nextZ->getMainGroup()->getSolidMask();

    auto& nextXGroup = nextX->getMainGroup();
    auto& nextYGroup = nextY->getMainGroup();
    auto& nextZGroup = nextZ->getMainGroup();
    
    std::unordered_set<BlockType> agregateTypesX = mergeMaskKeys(groupMaskTypes,nextXGroup->getMaskTypes());
    std::unordered_set<BlockType> agregateTypesY = mergeMaskKeys(groupMaskTypes,nextYGroup->getMaskTypes());
    std::unordered_set<BlockType> agregateTypesZ = mergeMaskKeys(groupMaskTypes,nextZGroup->getMaskTypes());

    std::unordered_set<BlockType> fullAgregate;
    fullAgregate.insert(agregateTypesX.begin(),agregateTypesX.end());
    fullAgregate.insert(agregateTypesY.begin(),agregateTypesY.end());
    fullAgregate.insert(agregateTypesZ.begin(),agregateTypesZ.end());

    for(int y = 0;y < size;y++){
        for(auto& key: agregateTypesX){
            const uint64_t localMaskRow = group->hasMask(key)     ? group->getMask(key).getRotatedAt(0,y) : 0ULL;
            const uint64_t otherMaskRow = nextXGroup->hasMask(key) ? nextXGroup->getMask(key).getRotatedAt(size - 1,y)   : 0ULL;
            
            uint64_t allFacesX =  (localMaskRow | otherMaskRow) & (solidRotated->getRow(0,y) ^ nextXSolid.getRotatedAt(size - 1,y));

            planesXforward[ (size_t)key][y] =  solidRotated->getRow(0,y) & allFacesX;
            planesXbackward[(size_t)key][y] =  nextXSolid.getRotatedAt(size - 1,y) & allFacesX;
        }

        for(auto& key: agregateTypesY){
            const uint64_t localMaskRow = group->hasMask(key)     ? group->getMask(key).getRow(y,0) : 0ULL;
            const uint64_t otherMaskRow = nextYGroup->hasMask(key) ? nextYGroup->getMask(key).getRow(y,size - 1)   : 0ULL;
            
            uint64_t allFacesY =  (localMaskRow | otherMaskRow) & (group->getSolidField().getRow(y,0) ^ nextYSolid.getRow(y,size - 1));

            planesYforward[ (size_t) key][y] = group->getSolidField().getRow(y,0) & allFacesY;
            planesYbackward[(size_t) key][y] = nextYSolid.getRow(y,size - 1) & allFacesY;
        }

        for(auto& key: agregateTypesZ){
            const uint64_t localMaskRow = group->hasMask(key)     ? group->getMask(key).getRow(0,y) : 0ULL;
            const uint64_t otherMaskRow = nextZGroup->hasMask(key) ? nextZGroup->getMask(key).getRow(size - 1,y)   : 0ULL;
            

            uint64_t allFacesZ =  (localMaskRow | otherMaskRow) & (group->getSolidField().getRow(0,y) ^ nextZSolid.getRow(size - 1,y));
                
            planesZforward[ (size_t) key][y] =  group->getSolidField().getRow(0,y) & allFacesZ;
            planesZbackward[(size_t) key][y] =  nextZSolid.getRow(size - 1,y) & allFacesZ;
        }
    }

    for(auto& key: fullAgregate){
        BlockType type = predefinedBlocks[key];
        if(!type.solid || type.billboard) continue;

        processFaces(greedyMeshPlane(planesXforward [static_cast<size_t>(key)], size), X, false, type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
        processFaces(greedyMeshPlane(planesXbackward[static_cast<size_t>(key)], size), X, true , type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);

        processFaces(greedyMeshPlane(planesYforward [static_cast<size_t>(key)], size), Y, false, type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
        processFaces(greedyMeshPlane(planesYbackward[static_cast<size_t>(key)], size), Y, true , type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);

        processFaces(greedyMeshPlane(planesZforward [static_cast<size_t>(key)], size), Z, false, type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
        processFaces(greedyMeshPlane(planesZbackward[static_cast<size_t>(key)], size), Z, true , type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
    }
    //std::cout << "Vertices:" << solidMesh.get()->getIndices().size() << std::endl;

    return solidMesh;*/
}