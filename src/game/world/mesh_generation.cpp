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
    auto start = std::chrono::high_resolution_clock::now();

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

#define AGREGATE_TYPES(axis) std::vector<BlockID> agregateTypes##axis = next##axis->getPresentTypes(); \
    agregateTypes##axis.insert(agregateTypes##axis.end(), group->getPresentTypes().begin(), group->getPresentTypes().end());

enum FaceDirection{
    X,Y,Z
};

static inline void processFaces(std::vector<ChunkMeshGenerator::Face> faces, FaceDirection direction, bool forward, BlockRegistry::RegisteredBlock* type, Mesh* mesh, int worldX, int worldY, int worldZ, int layer, float scale){
    std::array<glm::vec3, 4> vertices;
    int texture = 0;
    int normal;
    bool clockwise;

    float occlusion[4] = {0,0,0,0};
    
    glm::vec3 worldOffset = {worldX,worldY,worldZ};

    for(auto& face: faces){
        int faceWidth  = face.width;
        int faceHeight = face.height;
        switch(direction){
            case X:
                vertices = {
                    glm::vec3(layer + 1, face.y + face.height, face.x             ) * scale + worldOffset,
                    glm::vec3(layer + 1, face.y + face.height, face.x + face.width) * scale + worldOffset,
                    glm::vec3(layer + 1, face.y              , face.x + face.width) * scale + worldOffset,
                    glm::vec3(layer + 1, face.y              , face.x             ) * scale + worldOffset
                };
                texture = type->single_texture ? type->textures[0] : type->textures[4 + forward];
                normal = forward ? 3 : 2;
                clockwise = forward;
                break;
            case Y:
                vertices = {
                    glm::vec3(face.y              , layer + 1, face.x              ) * scale + worldOffset,
                    glm::vec3(face.y + face.height, layer + 1, face.x              ) * scale + worldOffset,
                    glm::vec3(face.y + face.height, layer + 1, face.x + face.width ) * scale + worldOffset,
                    glm::vec3(face.y              , layer + 1, face.x + face.width ) * scale + worldOffset
                };

                faceWidth = face.height;
                faceHeight = face.width;

                texture = type->single_texture ? type->textures[0] : type->textures[!forward];
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

                texture = type->single_texture ? type->textures[0] : type->textures[2 + forward];
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
            faceWidth,
            faceHeight
        );
    }
}   

std::unique_ptr<Mesh> ChunkMeshGenerator::generateChunkMesh(glm::ivec3 worldPosition, Chunk* group){
    auto start = std::chrono::high_resolution_clock::now();

    auto solidMesh = std::make_unique<Mesh>();
    if(!group){
        std::cout << "Empty group" << std::endl;
        return solidMesh;
    }
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

    for(int layer = 0; layer < size - 1;layer++){
        BlockBitPlanes<64> planesXforward = {0};
        BlockBitPlanes<64> planesXbackward = {0};

        BlockBitPlanes<64> planesYforward = {0};
        BlockBitPlanes<64> planesYbackward = {0};

        BlockBitPlanes<64> planesZforward = {0};
        BlockBitPlanes<64> planesZbackward = {0};
        
        for(int row = 0;row < size;row++){
            for(auto& [type,block,field]: group->getLayers()){
                auto* definition = blockRegistry.getRegisteredBlockByIndex(type);
                if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

                auto* rotatedField = field.getTransposed(&cache);

                if(!definition->transparent){
                    uint64_t allFacesX = (field.getRow(layer,row) | field.getRow(layer + 1,row)) & (group->getSolidField().getRow(layer,row) ^ group->getSolidField().getRow(layer + 1,row));
                    planesXforward[ (size_t) type][row] = group->getSolidField().getRow(layer,row)     & allFacesX;
                    planesXbackward[(size_t) type][row] = group->getSolidField().getRow(layer + 1,row) & allFacesX;

                    uint64_t allFacesY = (field.getRow(row,layer) | field.getRow(row,layer + 1)) & (group->getSolidField().getRow(row,layer) ^ group->getSolidField().getRow(row,layer + 1));
                    planesYforward[ (size_t) type][row] = group->getSolidField().getRow(row,layer)     & allFacesY;
                    planesYbackward[(size_t) type][row] = group->getSolidField().getRow(row,layer + 1) & allFacesY;

                    uint64_t allFacesZ = (rotatedField->getRow(layer,row) | rotatedField->getRow(layer + 1,row)) & (solidRotated->getRow(layer,row) ^ solidRotated->getRow(layer + 1,row));
                    planesZforward[ (size_t) type][row] = solidRotated->getRow(layer,row)     & allFacesZ;
                    planesZbackward[(size_t) type][row] = solidRotated->getRow(layer + 1,row) & allFacesZ;
                }
                else{
                    uint64_t allFacesX = (field.getRow(layer,row) ^ field.getRow(layer + 1,row)) & ~(group->getSolidField().getRow(layer,row) | group->getSolidField().getRow(layer + 1,row));
                    planesXforward[ (size_t) type][row] = field.getRow(layer,row) & allFacesX;
                    planesXbackward[(size_t) type][row] = field.getRow(layer + 1,row) & allFacesX;

                    uint64_t allFacesY = (field.getRow(row,layer) ^ field.getRow(row,layer + 1)) & ~(group->getSolidField().getRow(row,layer) | group->getSolidField().getRow(row,layer + 1));
                    planesYforward[ (size_t) type][row] = field.getRow(row,layer)     & allFacesY;
                    planesYbackward[(size_t) type][row] = field.getRow(row,layer + 1) & allFacesY;

                    uint64_t allFacesZ = (rotatedField->getRow(layer,row) ^ rotatedField->getRow(layer + 1,row)) & ~(solidRotated->getRow(layer,row) | solidRotated->getRow(layer + 1,row));
                    planesZforward[ (size_t) type][row] = rotatedField->getRow(layer,row)     & allFacesZ;
                    planesZbackward[(size_t) type][row] = rotatedField->getRow(layer + 1,row) & allFacesZ;
                }
            }
        }

        for(auto& type: group->getPresentTypes()){
            auto* definition = blockRegistry.getRegisteredBlockByIndex(type);
            if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;
            //std::cout << "Solving plane: " << getBlockTypeName(type) << std::endl;
            //for(int j = 0;j < 64;j++) std::cout << std::bitset<64>(planes[i][j]) << std::endl;

            processFaces(greedyMeshPlane(planesXforward [static_cast<size_t>(type)], size), X, true , definition, solidMesh.get(), worldX, worldY, worldZ, layer, scale);
            processFaces(greedyMeshPlane(planesXbackward[static_cast<size_t>(type)], size), X, false, definition, solidMesh.get(), worldX, worldY, worldZ, layer, scale);

            processFaces(greedyMeshPlane(planesYforward [static_cast<size_t>(type)], size), Y, true , definition, solidMesh.get(), worldX, worldY, worldZ, layer, scale);
            processFaces(greedyMeshPlane(planesYbackward[static_cast<size_t>(type)], size), Y, false, definition, solidMesh.get(), worldX, worldY, worldZ, layer, scale);

            processFaces(greedyMeshPlane(planesZforward [static_cast<size_t>(type)], size), Z, true , definition, solidMesh.get(), worldX, worldY, worldZ, layer, scale);
            processFaces(greedyMeshPlane(planesZbackward[static_cast<size_t>(type)], size), Z, false, definition, solidMesh.get(), worldX, worldY, worldZ, layer, scale);
        }
    }
    
    /*
        Mesh bilboards
    */

    for(auto& [type,block,field]: group->getLayers()){
        auto* definition = blockRegistry.getRegisteredBlockByIndex(type);
        if(!definition || definition->render_type != BlockRegistry::BILLBOARD) continue;

        for(int x = 0;x < size;x++) for(int y = 0;y < size;y++) for(int z = 0;z < size;z++){
            if(!field.get(x,y,z)) continue;

            int normal = 0;

            float occlusion[4] = {0,0,0,0};
            glm::vec3 worldOffset = {worldX,worldY,worldZ};
            glm::vec3 position = {x,y,z};
            
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
                static_cast<float>(definition->textures[0]), // Texture is the first one
                true,
                1,1
            );
            solidMesh->addQuadFaceGreedy(
                vertices1.data(),
                normal,
                occlusion,
                static_cast<float>(definition->textures[0]), // Texture is the first one
                false,
                1,1
            );

            solidMesh->addQuadFaceGreedy(
                vertices2.data(),
                normal,
                occlusion,
                static_cast<float>(definition->textures[0]), // Texture is the first one
                true,
                1,1
            );
            solidMesh->addQuadFaceGreedy(
                vertices2.data(),
                normal,
                occlusion,
                static_cast<float>(definition->textures[0]), // Texture is the first one
                false,
                1,1
            );
        }
    }
    /*
        Mesh cross chunk faces
    */
    BlockBitPlanes<64> planesXforward = {0};
    BlockBitPlanes<64> planesXbackward = {0};

    BlockBitPlanes<64> planesYforward = {0};
    BlockBitPlanes<64> planesYbackward = {0};

    BlockBitPlanes<64> planesZforward = {0};
    BlockBitPlanes<64> planesZbackward = {0};

    if(!world){
        std::cout << "Skipping neighbour generation." << std::endl;
        return solidMesh;
    }

    Chunk* nextX = world->getChunk(worldPosition - glm::ivec3{1,0,0});
    Chunk* nextY = world->getChunk(worldPosition - glm::ivec3{0,1,0});
    Chunk* nextZ = world->getChunk(worldPosition - glm::ivec3{0,0,1});

    //std::cout << nextX << " " << nextY << " " << nextZ << std::endl;

    if(!nextX || !nextY || !nextZ){
        std::cerr << "Mesh generating when chunks are missing?" << std::endl;
        return solidMesh;
    }

    BitField3D& nextXSolid = nextX->getSolidField();
    BitField3D& nextYSolid = nextY->getSolidField();

    auto* nextZSolidRotated = nextZ->getSolidField().getTransposed(&cache);
    
    AGREGATE_TYPES(X);
    AGREGATE_TYPES(Y);
    AGREGATE_TYPES(Z);

    std::vector<BlockID> fullAgregate = agregateTypesX;
    fullAgregate.insert(fullAgregate.begin(), agregateTypesY.begin(),agregateTypesY.end());
    fullAgregate.insert(fullAgregate.begin(), agregateTypesZ.begin(),agregateTypesZ.end());

    for(int row = 0;row < size;row++){
        for(auto& type: agregateTypesX){
            auto* definition = blockRegistry.getRegisteredBlockByIndex(type);
            if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

            const uint64_t localMaskRow = group->hasLayerOfType(type) ? group->getLayer(type).field.getRow(0,row) : 0ULL;
            const uint64_t otherMaskRow = nextX->hasLayerOfType(type) ? nextX->getLayer(type).field.getRow(size - 1,row)   : 0ULL;
            
            if(!definition->transparent){
                uint64_t allFacesX =  (localMaskRow | otherMaskRow) & (group->getSolidField().getRow(0,row) ^ nextXSolid.getRow(size - 1,row));
                    
                planesXforward[ (size_t) type][row] =  group->getSolidField().getRow(0,row) & allFacesX;
                planesXbackward[(size_t) type][row] =  nextXSolid.getRow(size - 1,row) & allFacesX;
            }
            else{
                uint64_t allFacesX =  (localMaskRow ^ otherMaskRow) & ~(group->getSolidField().getRow(0,row) | nextXSolid.getRow(size - 1,row));
                    
                planesXforward[ (size_t) type][row] =  localMaskRow & allFacesX;
                planesXbackward[(size_t) type][row] =  otherMaskRow & allFacesX;
            }
        }
        
        for(auto& type: agregateTypesY){
            auto* definition = blockRegistry.getRegisteredBlockByIndex(type);
            if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

            const uint64_t localMaskRow = group->hasLayerOfType(type) ? group->getLayer(type).field.getRow(row,0) : 0ULL;
            const uint64_t otherMaskRow = nextY->hasLayerOfType(type) ? nextY->getLayer(type).field.getRow(row,size - 1)   : 0ULL;
            
            if(!definition->transparent){
                uint64_t allFacesY =  (localMaskRow | otherMaskRow) & (group->getSolidField().getRow(row,0) ^ nextYSolid.getRow(row,size - 1));

                planesYforward[ (size_t) type][row] = group->getSolidField().getRow(row,0) & allFacesY;
                planesYbackward[(size_t) type][row] = nextYSolid.getRow(row,size - 1) & allFacesY;
            }
            else{
                uint64_t allFacesY =  (localMaskRow ^ otherMaskRow) & ~(group->getSolidField().getRow(row,0) | nextYSolid.getRow(row,size - 1));

                planesYforward[ (size_t) type][row] = localMaskRow & allFacesY;
                planesYbackward[(size_t) type][row] = otherMaskRow & allFacesY;
            }
        }

        for(auto& type: agregateTypesZ){
            auto* definition = blockRegistry.getRegisteredBlockByIndex(type);
            if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

            uint64_t localMaskRow = 0ULL;
            if(group->hasLayerOfType(type)) localMaskRow = group->getLayer(type).field.getTransposed(&cache)->getRow(0,row);
            
            uint64_t otherMaskRow = 0ULL;
            if(nextZ->hasLayerOfType(type)) otherMaskRow = nextZ->getLayer(type).field.getTransposed(&cache)->getRow(size - 1,row); 
            
            if(!definition->transparent){
                uint64_t allFacesX =  (localMaskRow | otherMaskRow) & (solidRotated->getRow(0,row) ^ nextZSolidRotated->getRow(size - 1,row));

                planesZforward[ (size_t)type][row] =  solidRotated->getRow(0,row) & allFacesX;
                planesZbackward[(size_t)type][row] =  nextZSolidRotated->getRow(size - 1,row) & allFacesX;
            }
            else{
                uint64_t allFacesX =  (localMaskRow ^ otherMaskRow) & ~(solidRotated->getRow(0,row) | nextZSolidRotated->getRow(size - 1,row));

                planesZforward[ (size_t)type][row] =  localMaskRow & allFacesX;
                planesZbackward[(size_t)type][row] =  otherMaskRow & allFacesX;
            }
        }
    }

    for(auto& type: fullAgregate){
        auto* definition = blockRegistry.getRegisteredBlockByIndex(type);
        if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

        processFaces(greedyMeshPlane(planesXforward [static_cast<size_t>(type)], size), X, false, definition, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
        processFaces(greedyMeshPlane(planesXbackward[static_cast<size_t>(type)], size), X, true , definition, solidMesh.get(), worldX, worldY, worldZ, -1, scale);

        processFaces(greedyMeshPlane(planesYforward [static_cast<size_t>(type)], size), Y, false, definition, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
        processFaces(greedyMeshPlane(planesYbackward[static_cast<size_t>(type)], size), Y, true , definition, solidMesh.get(), worldX, worldY, worldZ, -1, scale);

        processFaces(greedyMeshPlane(planesZforward [static_cast<size_t>(type)], size), Z, false, definition, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
        processFaces(greedyMeshPlane(planesZbackward[static_cast<size_t>(type)], size), Z, true , definition, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
    }
    //std::cout << "Vertices:" << solidMesh.get()->getIndices().size() << std::endl;
    
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //std::cout << "Generated chunk mesh (" <<  worldPosition.x << "," << worldPosition.y << "," << worldPosition.z << ") in: " << duration << " microseconds" << std::endl;

    return solidMesh;
}