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
    std::vector<Face> out{};

    /*
        Moves bits to from the right to the left

        if size = 4
        00001111
        then it gets converted to this for meshing (because count_leading_zeros starts from the left )
        11110000
    */
    if(size != 64){
        int size_to_real_size_adjustment = 64 - size;
        for(int i = 0;i < size;i++) rows[i] <<= size_to_real_size_adjustment;
    }

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

std::tuple<ChunkMeshGenerator::OccludedPlane, bool, ChunkMeshGenerator::OccludedPlane, bool> ChunkMeshGenerator::segregatePlane(
    OccludedPlane& source_plane,
    OcclusionPlane& occlusion_plane,
    std::array<bool,4> affects,
    glm::ivec2 lookup_offset
){
    OccludedPlane true_plane  = {source_plane.occlusion};
    OccludedPlane false_plane = {source_plane.occlusion};

    true_plane.occlusion[0] += affects[0];
    true_plane.occlusion[1] += affects[1];
    true_plane.occlusion[2] += affects[2];
    true_plane.occlusion[3] += affects[3];

    bool true_plane_empty  = true;
    bool false_plane_empty = true;

    for(int i = source_plane.start;i < source_plane.end;i++){
        if(source_plane.plane[i] == 0) continue;

        int lookup_y = (i + lookup_offset.y) + 1; // occlusion plane is offset relative to source plane so +1

        uint64_t occlusion_row = occlusion_plane.rows[lookup_y];
        
        //occlusion_row = 
        //    (lookup_offset.x > 0) ? 
        //        (occlusion_row <<  lookup_offset.x): 
        //        (occlusion_row >> -lookup_offset.x);

        if(lookup_offset.x > 0){
            occlusion_row <<=  lookup_offset.x;
            
            if     (lookup_y == 0)  occlusion_row |= occlusion_plane.top_right_corner;
            else if(lookup_y == 65) occlusion_row |= occlusion_plane.bottom_right_corner;
            else occlusion_row |= ((occlusion_plane.right >> (lookup_y - 1)) & 1ULL); 
        }
        else{
            occlusion_row >>= -lookup_offset.x;

            if     (lookup_y == 0)  occlusion_row |= occlusion_plane.top_left_corner;
            else if(lookup_y == 65) occlusion_row |= occlusion_plane.bottom_left_corner;
            else occlusion_row |= ((occlusion_plane.left >> (lookup_y - 1)) & 1ULL);
        }

        true_plane .plane[i] = source_plane.plane[i] &  occlusion_row;
        false_plane.plane[i] = source_plane.plane[i] & ~occlusion_row;

        if(true_plane .plane[i] != 0ULL){
            true_plane.end = i + 1;
            true_plane_empty  = false;
        }
        if(false_plane.plane[i] != 0ULL){
            false_plane.end = i + 1;
            false_plane_empty = false;
        }

        if(true_plane_empty) true_plane.start = i;
        if(false_plane_empty) false_plane.start = i;
    }

    return {
        true_plane,
        true_plane_empty,
        false_plane,
        false_plane_empty
    };
}


struct OcclusionOffset{
    glm::ivec2 offset;
    std::array<bool, 4> affects;
};
const static std::array<OcclusionOffset, 8> occlusion_offsets = {
    // Whole sides
    OcclusionOffset{{ 1, 0}, {0,1,1,0}},
    OcclusionOffset{{-1, 0}, {1,0,0,1}},
    OcclusionOffset{{ 0, 1}, {0,0,1,1}},
    OcclusionOffset{{ 0,-1}, {1,1,0,0}},
    // Corners 
    OcclusionOffset{{-1, 1}, {0,0,0,1}},
    OcclusionOffset{{ 1, 1}, {0,0,1,0}},
    OcclusionOffset{{ 1,-1}, {0,1,0,0}},
    OcclusionOffset{{-1,-1}, {1,0,0,0}}
};

std::vector<ChunkMeshGenerator::OccludedPlane> ChunkMeshGenerator::calculatePlaneAmbientOcclusion(BitPlane<64>& source_plane, OcclusionPlane& occlusion_plane){
    std::vector<OccludedPlane> planes{};
    planes.reserve(8);
    planes.push_back({{0,0,0,0}, source_plane});
    
    for(auto& [offset, affects]: occlusion_offsets){
        for(auto& plane: planes){
            auto [plane_1, plane_1_empty, plane_2, plane_2_empty] = segregatePlane(plane, occlusion_plane, affects, offset);

            if(plane_1_empty){
                plane = plane_2;
                continue;
            }

            plane = plane_1;
            
            if(plane_2_empty) continue;
            planes.push_back(plane_2);
        }
    }

    return planes;
}

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
    std::array<float, 4>& occlusion
){
    glm::vec3 face_position;
    int texture = 0;
    int normal;
    bool clockwise;

    bool texture_index = direction == InstancedMesh::Backward;

    for(auto& face: faces){
        int faceWidth  = face.width;
        int faceHeight = face.height;

        switch(face_type){
            case InstancedMesh::X_ALIGNED:
                face_position = glm::vec3(layer + 1, face.y + face.height, face.x) + world_position;

                texture = type->single_texture ? type->textures[0] : type->textures[4 + texture_index ];

                break;
            case InstancedMesh::Y_ALIGNED:
                face_position = glm::vec3(face.y              , layer + 1, face.x              ) + world_position;

                faceWidth = face.height;
                faceHeight = face.width;

                texture = type->single_texture ? type->textures[0] : type->textures[texture_index];
                break;
            case InstancedMesh::Z_ALIGNED:
                face_position = glm::vec3(face.x              , face.y + face.height, layer + 1) + world_position;
  
                texture = type->single_texture ? type->textures[0] : type->textures[2 + texture_index ];
                break;
        }

        mesh->addQuadFace(face_position, faceWidth, faceHeight, texture, face_type, direction, occlusion);
    }
}   

void ChunkMeshGenerator::proccessOccludedFaces(
    BitPlane<64>& source_plane,
    OcclusionPlane& occlusion_plane,
    
    InstancedMesh::FaceType face_type,
    InstancedMesh::Direction direction,
    BlockRegistry::BlockPrototype* type,
    InstancedMesh* mesh, 
    glm::vec3 world_position,
    int layer
){
    auto processed_planes = calculatePlaneAmbientOcclusion(source_plane, occlusion_plane);

    for(auto& plane: processed_planes){
        processFaces(greedyMeshPlane(plane.plane), face_type, direction, type, mesh, world_position, layer, plane.occlusion);
    }
}


std::unique_ptr<InstancedMesh> ChunkMeshGenerator::generateChunkMesh(glm::ivec3 worldPosition, Chunk* group){
    //ScopeTimer timer("Generated mesh");

    auto solidMesh = std::make_unique<InstancedMesh>();
    if(!group){
        std::cerr << "Empty group" << std::endl;
        return solidMesh;
    }

    if(!world){
        std::cerr << "No world assigned, quitting mesh generation." << std::endl;
        return solidMesh;
    }

    Chunk* nextX = world->getChunk(worldPosition - glm::ivec3{1,0,0});
    Chunk* nextY = world->getChunk(worldPosition - glm::ivec3{0,1,0});
    Chunk* nextZ = world->getChunk(worldPosition - glm::ivec3{0,0,1});

    Chunk* forwardX = world->getChunk(worldPosition + glm::ivec3{1,0,0});
    Chunk* forwardY = world->getChunk(worldPosition + glm::ivec3{0,1,0});
    Chunk* forwardZ = world->getChunk(worldPosition + glm::ivec3{0,0,1});

    
    if(!nextX || !nextY || !nextZ || !forwardX || !forwardY || !forwardZ){
        std::cerr << "Mesh generating when chunks are missing?" << std::endl;
        return solidMesh;
    }
    
    //this->solidMesh->setVertexFormat(VertexFormat({3,1,2,1,1}));  // Unused

    glm::vec3 world_position = worldPosition * CHUNK_SIZE;
    int size = CHUNK_SIZE;

    // Occlusion planes have to reach into other chunks
    std::array<OcclusionPlane, 64> occlusionPlanesX{};
    std::array<OcclusionPlane, 64> occlusionPlanesY{};
    std::array<OcclusionPlane, 64> occlusionPlanesZ{};
    
    { // Scope becuse transposed field doesnt neccesairly remain valid when new ones are created
        auto& solidField = group->getSolidField();
        auto* solidRotated = group->getSolidField().getTransposed();
        // 64 planes internale 65th external

        for(int x = 0;x < 64;x++) for(int y = 0;y < 64;y++){
            occlusionPlanesX[x].rows[y + 1] = solidField.getRow(x,y);    
            occlusionPlanesY[y].rows[x + 1] = solidField.getRow(x,y);
            occlusionPlanesZ[x].rows[y + 1] = solidRotated->getRow(x,y);
        }   

        auto* left  = nextX->getSolidField().getTransposed(); 
        auto* right = forwardX->getSolidField().getTransposed();
        auto& back  = nextZ->getSolidField();
        auto& front = forwardZ->getSolidField();

        /*for(int y = 0;y < 64;y++){
            occlusionPlanesY[y].rows[0]  = back .getRow(63,y);
            occlusionPlanesY[y].rows[65] = front.getRow(0 ,y);
            occlusionPlanesY[y].left  = left->getRow(63, y);
            occlusionPlanesY[y].right = right->getRow(0,y);
        }*/

        for(int y = 0;y < 64;y++){

        }
    }
    //timer.timestamp("Generated occlusion planes");

    //std::cout << "Scale: " << scale << " size: " << size << std::endl;
    /*
        Mesh chunk faces
    */

    for(int layer = 0; layer < size - 1;layer++){
        BlockBitPlanes<64> planesXforward = {};
        BlockBitPlanes<64> planesXbackward = {};

        BlockBitPlanes<64> planesYforward = {};
        BlockBitPlanes<64> planesYbackward = {};

        BlockBitPlanes<64> planesZforward = {};
        BlockBitPlanes<64> planesZbackward = {};
        
        for(int row = 0;row < size;row++){
            for(auto& field_layer: group->getLayers()){
                auto& [type,block,_field] = field_layer;
                auto& field = field_layer.field();

                auto* definition = global_block_registry.getBlockPrototypeByIndex(type);
                if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

                auto* rotatedField = field.getTransposed();
                auto* solidRotated = group->getSolidField().getTransposed();

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
            auto* definition = global_block_registry.getBlockPrototypeByIndex(type);
            if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;
            //std::cout << "Solving plane: " << getBlockTypeName(type) << std::endl;
            //for(int j = 0;j < 64;j++) std::cout << std::bitset<64>(planes[i][j]) << std::endl;

            proccessOccludedFaces(planesXforward [static_cast<size_t>(type)], occlusionPlanesX[layer + 1], InstancedMesh::X_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, layer);
            proccessOccludedFaces(planesXbackward[static_cast<size_t>(type)], occlusionPlanesX[layer    ], InstancedMesh::X_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, layer);

            proccessOccludedFaces(planesYforward [static_cast<size_t>(type)], occlusionPlanesY[layer + 1], InstancedMesh::Y_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, layer);
            proccessOccludedFaces(planesYbackward[static_cast<size_t>(type)], occlusionPlanesY[layer    ], InstancedMesh::Y_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, layer);

            proccessOccludedFaces(planesZforward [static_cast<size_t>(type)], occlusionPlanesZ[layer + 1], InstancedMesh::Z_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, layer);
            proccessOccludedFaces(planesZbackward[static_cast<size_t>(type)], occlusionPlanesZ[layer    ], InstancedMesh::Z_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, layer);
        }
    }

    ////timer.timestamp("Inner faces");
    
    /*
        Mesh billboards
    */

    for(auto& field_layer: group->getLayers()){
        auto& [type,block,_field] = field_layer;
        auto& field = field_layer.field();
        
        auto* definition = global_block_registry.getBlockPrototypeByIndex(type);
        if(!definition || definition->render_type != BlockRegistry::BILLBOARD) continue;

        for(int x = 0;x < size;x++) for(int y = 0;y < size;y++) for(int z = 0;z < size;z++){
            if(!field.get(x,y,z)) continue;

            glm::vec3 position = glm::vec3{x,y,z} + world_position;

            solidMesh->addQuadFace(position,1,1,definition->textures[0], InstancedMesh::BILLBOARD, InstancedMesh::Forward, {0,0,0,0});
        }
    }

    ////timer.timestamp("Billboards");
    /*
        Mesh cross chunk faces
    */
    BlockBitPlanes<64> planesXforward = {};
    BlockBitPlanes<64> planesXbackward = {};

    BlockBitPlanes<64> planesYforward = {};
    BlockBitPlanes<64> planesYbackward = {};

    BlockBitPlanes<64> planesZforward = {};
    BlockBitPlanes<64> planesZbackward = {};

    std::array<float,4> occlusion = {0,0,0,0};
    //std::cout << nextX << " " << nextY << " " << nextZ << std::endl;

    AGREGATE_TYPES(X);
    AGREGATE_TYPES(Y);
    AGREGATE_TYPES(Z);

    std::vector<BlockID> fullAgregate = agregateTypesX;
    fullAgregate.insert(fullAgregate.begin(), agregateTypesY.begin(),agregateTypesY.end());
    fullAgregate.insert(fullAgregate.begin(), agregateTypesZ.begin(),agregateTypesZ.end());

    for(int row = 0;row < size;row++){
        for(auto& type: agregateTypesX){
            auto* definition = global_block_registry.getBlockPrototypeByIndex(type);
            if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;
            
            BitField3D& nextXSolid = nextX->getSolidField();

            const uint64_t localMaskRow = group->hasLayerOfType(type) ? group->getLayer(type).field().getRow(0,row) : 0ULL;
            const uint64_t otherMaskRow = nextX->hasLayerOfType(type) ? nextX->getLayer(type).field().getRow(size - 1,row)   : 0ULL;
            
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
            auto* definition = global_block_registry.getBlockPrototypeByIndex(type);
            if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

            BitField3D& nextYSolid = nextY->getSolidField();

            const uint64_t localMaskRow = group->hasLayerOfType(type) ? group->getLayer(type).field().getRow(row,0) : 0ULL;
            const uint64_t otherMaskRow = nextY->hasLayerOfType(type) ? nextY->getLayer(type).field().getRow(row,size - 1)   : 0ULL;
            
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
            auto* definition = global_block_registry.getBlockPrototypeByIndex(type);
            if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

            auto* nextZSolidRotated = nextZ->getSolidField().getTransposed();

            uint64_t localMaskRow = 0ULL;
            if(group->hasLayerOfType(type)) localMaskRow = group->getLayer(type).field().getTransposed()->getRow(0,row);
            
            uint64_t otherMaskRow = 0ULL;
            if(nextZ->hasLayerOfType(type)) otherMaskRow = nextZ->getLayer(type).field().getTransposed()->getRow(size - 1,row); 

            auto* solidRotated = group->getSolidField().getTransposed();
            
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
        auto* definition = global_block_registry.getBlockPrototypeByIndex(type);
        if(!definition || definition->render_type != BlockRegistry::FULL_BLOCK) continue;

        processFaces(greedyMeshPlane(planesXforward [static_cast<size_t>(type)], size), InstancedMesh::X_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, -1, occlusion);
        processFaces(greedyMeshPlane(planesXbackward[static_cast<size_t>(type)], size), InstancedMesh::X_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, -1, occlusion);

        processFaces(greedyMeshPlane(planesYforward [static_cast<size_t>(type)], size), InstancedMesh::Y_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, -1, occlusion);
        processFaces(greedyMeshPlane(planesYbackward[static_cast<size_t>(type)], size), InstancedMesh::Y_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, -1, occlusion);

        processFaces(greedyMeshPlane(planesZforward [static_cast<size_t>(type)], size), InstancedMesh::Z_ALIGNED, InstancedMesh::Backward, definition, solidMesh.get(), world_position, -1, occlusion);
        processFaces(greedyMeshPlane(planesZbackward[static_cast<size_t>(type)], size), InstancedMesh::Z_ALIGNED, InstancedMesh::Forward , definition, solidMesh.get(), world_position, -1, occlusion);
    }
    //std::cout << "Vertices:" << solidMesh.get()->getIndices().size() << std::endl;
    
    
    solidMesh->shrink();
    return solidMesh;
}