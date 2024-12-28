#include <game/world/mesh_generation.hpp>


void ChunkMeshGenerator::addToChunkMeshLoadingQueue(glm::ivec3 position, std::unique_ptr<InstancedMesh> mesh){
    std::lock_guard<std::mutex> lock(meshLoadingMutex);
    meshLoadingQueue.push({position,std::move(mesh)});
}
void ChunkMeshGenerator::loadMeshFromQueue(ChunkMeshRegistry&  buffer){
    std::lock_guard<std::mutex> lock(meshLoadingMutex);
    if(meshLoadingQueue.empty()) return;
    auto& [position,mesh] = meshLoadingQueue.front();

    if(buffer.addMesh(mesh.get(), position)){
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

    buffer.addMesh(solid_mesh.get(), chunk->getWorldPosition());
}

/*
    Generate greedy meshed faces from a plane of bits
*/
std::vector<ChunkMeshGenerator::Face> ChunkMeshGenerator::greedyMeshPlane(BitPlane<64> rows, int size){
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

static inline void processFaces(
    std::vector<ChunkMeshGenerator::Face> faces,
    InstancedMesh::FaceType face_type,
    InstancedMesh::Direction direction,
    BlockRegistry::BlockPrototype* type,
    InstancedMesh* mesh, 
    glm::vec3 world_position,
    int layer,
    float scale
){
    glm::vec3 face_position;
    int texture = 0;
    int normal;
    bool clockwise;

    bool texture_index = direction == InstancedMesh::Backward;

    float occlusion[4] = {0,0,0,0};

    for(auto& face: faces){
        int faceWidth  = face.width;
        int faceHeight = face.height;

        switch(face_type){
            case InstancedMesh::X_ALIGNED:
                face_position = glm::vec3(layer + 1, face.y + face.height, face.x) * scale + world_position;

                texture = type->single_texture ? type->textures[0] : type->textures[4 + texture_index ];

                break;
            case InstancedMesh::Y_ALIGNED:
                face_position = glm::vec3(face.y              , layer + 1, face.x              ) * scale + world_position;

                faceWidth = face.height;
                faceHeight = face.width;

                texture = type->single_texture ? type->textures[0] : type->textures[texture_index];
                break;
            case InstancedMesh::Z_ALIGNED:
                face_position = glm::vec3(face.x              , face.y + face.height, layer + 1) * scale + world_position;
  
                texture = type->single_texture ? type->textures[0] : type->textures[2 + texture_index ];
                break;
        }

        mesh->addQuadFace(face_position, faceWidth, faceHeight, texture, face_type, direction);
    }
}   

std::unique_ptr<InstancedMesh> ChunkMeshGenerator::generateChunkMesh(glm::ivec3 worldPosition, Chunk* group){
    auto start = std::chrono::high_resolution_clock::now();

    auto solidMesh = std::make_unique<InstancedMesh>();
    if(!group){
        std::cout << "Empty group" << std::endl;
        return solidMesh;
    }
    //this->solidMesh->setVertexFormat(VertexFormat({3,1,2,1,1}));  // Unused

    glm::vec3 world_position = worldPosition * CHUNK_SIZE;

    int size = CHUNK_SIZE;
    float scale = 1.0;

    //std::cout << "Scale: " << scale << " size: " << size << std::endl;
    /*
        Mesh chunk faces
    */

    auto* solidRotated = group->getSolidField().getTransposed();

    for(int layer = 0; layer < size - 1;layer++){
        BlockBitPlanes<64> planesXforward = {};
        BlockBitPlanes<64> planesXbackward = {};

        BlockBitPlanes<64> planesYforward = {};
        BlockBitPlanes<64> planesYbackward = {};

        BlockBitPlanes<64> planesZforward = {};
        BlockBitPlanes<64> planesZbackward = {};
        
        for(int row = 0;row < size;row++){
            for(auto& [type,block,field]: group->getLayers()){
                auto* definition = blockRegistry.getBlockPrototypeByIndex(type);
                if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

                auto* rotatedField = field.getTransposed();

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
            auto* definition = blockRegistry.getBlockPrototypeByIndex(type);
            if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;
            //std::cout << "Solving plane: " << getBlockTypeName(type) << std::endl;
            //for(int j = 0;j < 64;j++) std::cout << std::bitset<64>(planes[i][j]) << std::endl;

            processFaces(greedyMeshPlane(planesXforward [static_cast<size_t>(type)], size), InstancedMesh::X_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, layer, scale);
            processFaces(greedyMeshPlane(planesXbackward[static_cast<size_t>(type)], size), InstancedMesh::X_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, layer, scale);

            processFaces(greedyMeshPlane(planesYforward [static_cast<size_t>(type)], size), InstancedMesh::Y_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, layer, scale);
            processFaces(greedyMeshPlane(planesYbackward[static_cast<size_t>(type)], size), InstancedMesh::Y_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, layer, scale);

            processFaces(greedyMeshPlane(planesZforward [static_cast<size_t>(type)], size), InstancedMesh::Z_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, layer, scale);
            processFaces(greedyMeshPlane(planesZbackward[static_cast<size_t>(type)], size), InstancedMesh::Z_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, layer, scale);
        }
    }
    
    /*
        Mesh billboards
    */

    for(auto& [type,block,field]: group->getLayers()){
        auto* definition = blockRegistry.getBlockPrototypeByIndex(type);
        if(!definition || definition->render_type != BlockRegistry::BILLBOARD) continue;

        for(int x = 0;x < size;x++) for(int y = 0;y < size;y++) for(int z = 0;z < size;z++){
            if(!field.get(x,y,z)) continue;

            glm::vec3 position = glm::vec3{x,y,z} * scale + world_position;

            solidMesh->addQuadFace(position,1,1,definition->textures[0], InstancedMesh::BILLBOARD, InstancedMesh::Forward);
        }
    }
    /*
        Mesh cross chunk faces
    */
    BlockBitPlanes<64> planesXforward = {};
    BlockBitPlanes<64> planesXbackward = {};

    BlockBitPlanes<64> planesYforward = {};
    BlockBitPlanes<64> planesYbackward = {};

    BlockBitPlanes<64> planesZforward = {};
    BlockBitPlanes<64> planesZbackward = {};

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

    auto* nextZSolidRotated = nextZ->getSolidField().getTransposed();
    
    AGREGATE_TYPES(X);
    AGREGATE_TYPES(Y);
    AGREGATE_TYPES(Z);

    std::vector<BlockID> fullAgregate = agregateTypesX;
    fullAgregate.insert(fullAgregate.begin(), agregateTypesY.begin(),agregateTypesY.end());
    fullAgregate.insert(fullAgregate.begin(), agregateTypesZ.begin(),agregateTypesZ.end());

    for(int row = 0;row < size;row++){
        for(auto& type: agregateTypesX){
            auto* definition = blockRegistry.getBlockPrototypeByIndex(type);
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
            auto* definition = blockRegistry.getBlockPrototypeByIndex(type);
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
            auto* definition = blockRegistry.getBlockPrototypeByIndex(type);
            if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

            uint64_t localMaskRow = 0ULL;
            if(group->hasLayerOfType(type)) localMaskRow = group->getLayer(type).field.getTransposed()->getRow(0,row);
            
            uint64_t otherMaskRow = 0ULL;
            if(nextZ->hasLayerOfType(type)) otherMaskRow = nextZ->getLayer(type).field.getTransposed()->getRow(size - 1,row); 
            
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
        auto* definition = blockRegistry.getBlockPrototypeByIndex(type);
        if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

        processFaces(greedyMeshPlane(planesXforward [static_cast<size_t>(type)], size), InstancedMesh::X_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, -1, scale);
        processFaces(greedyMeshPlane(planesXbackward[static_cast<size_t>(type)], size), InstancedMesh::X_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, -1, scale);

        processFaces(greedyMeshPlane(planesYforward [static_cast<size_t>(type)], size), InstancedMesh::Y_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, -1, scale);
        processFaces(greedyMeshPlane(planesYbackward[static_cast<size_t>(type)], size), InstancedMesh::Y_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, -1, scale);

        processFaces(greedyMeshPlane(planesZforward [static_cast<size_t>(type)], size), InstancedMesh::Z_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, -1, scale);
        processFaces(greedyMeshPlane(planesZbackward[static_cast<size_t>(type)], size), InstancedMesh::Z_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, -1, scale);
    }
    //std::cout << "Vertices:" << solidMesh.get()->getIndices().size() << std::endl;
    
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //std::cout << "Generated chunk mesh (" <<  worldPosition.x << "," << worldPosition.y << "," << worldPosition.z << ") in: " << duration << " microseconds" << std::endl;
    solidMesh->shrink();
    return solidMesh;
}