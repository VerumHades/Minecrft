#include <game/chunk.hpp>

static inline void fillChunkLevel(Chunk& chunk, uint32_t y, Block value){
    for(int x = 0; x < CHUNK_SIZE;x++) for(int z = 0;z < CHUNK_SIZE;z++){
        chunk.setBlock(x,y,z, value);
    }
}

Block::Block(){
    this->type = BlockTypes::Air;
}
Block::Block(BlockTypes type): type(type){
    this->type = type;
}

World& Chunk::getWorld(){
    return this->world;
}

static Block airBlock = {BlockTypes::Air};
Block* Chunk::getBlock(uint32_t x, uint32_t y, uint32_t z){
    if(x >= CHUNK_SIZE) return nullptr;
    if(y >= CHUNK_SIZE) return nullptr;
    if(z >= CHUNK_SIZE) return nullptr;
    if(!isMainGroupOfSize(64)) return nullptr;
    
    uint64_t checkMask = (1ULL << (63 - x));
    
    for(auto& [key,mask]: currentGroup->getMasks()){
        uint64_t row = mask.getAt(z,y);

        if(row & checkMask){
            return &mask.getBlock();
        }
    }

    return &airBlock;
}

bool Chunk::setBlock(uint32_t x, uint32_t y, uint32_t z, Block value){
    if(x >= CHUNK_SIZE) return false;
    if(y >= CHUNK_SIZE) return false;
    if(z >= CHUNK_SIZE) return false;
    if(!isMainGroupOfSize(64)) return false;

    uint64_t currentMask = (1ULL << (63 - x));

    if(value.type != BlockTypes::Air){
        if(currentGroup->getMasks().count(value.type) == 0){
            DynamicChunkMask mask = {64,value};
            currentGroup->setMask(value.type, mask);
        }

        currentGroup->getMask(value.type).set(x,y,z);
    }

    for(auto& [key,mask]: currentGroup->getMasks()){
        uint64_t row = mask.getAt(z,y);

        if(!(row & currentMask)) continue;
        if(mask.getBlock().type == value.type) continue;

        mask.reset(x,y,z);
        break;
    }

    if(getBlockType(&value).solid) currentGroup->getSolidMask().set(x,y,z);
    else currentGroup->getSolidMask().reset(x,y,z);

    return true;
}

void Chunk::syncGenerateAsyncUploadMesh(){
    generateMeshes();

    //End time point
    //auto end = std::chrono::high_resolution_clock::now();

    //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //std::cout << "Execution time: " << duration << " microseconds" << std::endl;
    if(solidMesh->getVertices().size() == 0) generatedEmptyMesh = true;
    
    world.addToChunkMeshLoadingQueue(this->worldPosition, std::move(this->solidMesh));
}

void Chunk::asyncGenerateAsyncUploadMesh(ThreadPool& pool, bool reload){
    if(
        !world.getChunk(worldPosition.x - 1,worldPosition.y,worldPosition.z) ||
        !world.getChunk(worldPosition.x,worldPosition.y - 1,worldPosition.z) ||
        !world.getChunk(worldPosition.x,worldPosition.y,worldPosition.z - 1) 
    ) return;

    if(reload) generatedEmptyMesh = false;

    if(
        !meshGenerating && !generatedEmptyMesh && !isEmpty()
    ){
        bool success = pool.deploy([this](){
            syncGenerateAsyncUploadMesh();
            meshGenerating = false;
        });
        if(!success) return;

        meshGenerating = true;
        //generateChunkMeshThread(chunk);
        return;
    } 
}

void Chunk::syncGenerateSyncUploadMesh(ChunkMeshRegistry& buffer){
    generateMeshes();

    if(solidMesh->getVertices().size() == 0) generatedEmptyMesh = true;
    
    if(buffer.isChunkLoaded(worldPosition)) buffer.updateMesh(*solidMesh, getWorldPosition());
    else buffer.addMesh(*solidMesh, getWorldPosition());

    solidMesh = nullptr;
}

/*
    Generate greedy meshed faces from a plane of bits
*/
std::vector<Face> greedyMeshPlane(std::array<uint_t<64>, 64> rows, int size){
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
        uint_t<64> row = rows[currentRow];
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
        uint_t<64> mask = ~0ULL;

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

static inline std::unordered_set<BlockTypes> mergeMaskKeys(const std::unordered_set<BlockTypes>& a, const std::unordered_set<BlockTypes>& b){
    std::unordered_set<BlockTypes> out;
    for(auto& key: a) out.emplace(key);
    for(auto& key: b) out.emplace(key);
    return out;
}

enum FaceDirection{
    X,Y,Z
};

static inline void processFaces(std::vector<Face> faces, FaceDirection direction, bool forward, BlockType& type, Mesh* mesh, int worldX, int worldY, int worldZ, int layer, float scale){
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

std::unique_ptr<Mesh> generateChunkMesh(World& world, glm::ivec3 worldPosition, DynamicChunkContents* group){
    auto solidMesh = std::make_unique<Mesh>();
    //this->solidMesh->setVertexFormat(VertexFormat({3,1,2,1,1}));  // Unused
    float worldX = worldPosition.x * CHUNK_SIZE;
    float worldY = worldPosition.y * CHUNK_SIZE;
    float worldZ = worldPosition.z * CHUNK_SIZE;

    int size = group->getSize();
    float scale = static_cast<float>(CHUNK_SIZE) / static_cast<float>(size);

    //std::cout << "Scale: " << scale << " size: " << size << std::endl;

    auto groupMaskTypes = group->getMaskTypes();

    /*
        Mesh chunk faces
    */
    for(int z = 0; z < size - 1;z++){
        BlockBitPlanes<64> planesXforward = {0};
        BlockBitPlanes<64> planesXbackward = {0};

        BlockBitPlanes<64> planesYforward = {0};
        BlockBitPlanes<64> planesYbackward = {0};

        BlockBitPlanes<64> planesZforward = {0};
        BlockBitPlanes<64> planesZbackward = {0};
        
        for(int y = 0;y < size;y++){
            for(auto& key: groupMaskTypes){
                auto& mask = group->getMask(key);

                uint_t<64> allFacesX = (mask.getRotatedAt(z,y) | mask.getRotatedAt(z + 1,y)) & (group->getSolidMask().getRotatedAt(z,y) ^ group->getSolidMask().getRotatedAt(z + 1,y));
                planesXforward[ (size_t) mask.getBlock().type][y] = group->getSolidMask().getRotatedAt(z,y) & allFacesX;
                planesXbackward[(size_t) mask.getBlock().type][y] = group->getSolidMask().getRotatedAt(z + 1,y) & allFacesX;

                uint_t<64> allFacesY = (mask.getAt(y,z) | mask.getAt(y,z + 1)) & (group->getSolidMask().getAt(y,z) ^ group->getSolidMask().getAt(y,z + 1));
                planesYforward[ (size_t) mask.getBlock().type][y] = group->getSolidMask().getAt(y,z) & allFacesY;
                planesYbackward[(size_t) mask.getBlock().type][y] = group->getSolidMask().getAt(y,z + 1) & allFacesY;

                uint_t<46> allFacesZ = (mask.getAt(z,y) | mask.getAt(z + 1,y)) & (group->getSolidMask().getAt(z,y) ^ group->getSolidMask().getAt(z + 1,y));
                planesZforward[ (size_t) mask.getBlock().type][y] = group->getSolidMask().getAt(z,y) & allFacesZ;
                planesZbackward[(size_t) mask.getBlock().type][y] = group->getSolidMask().getAt(z + 1,y) & allFacesZ;
            }
        }

        for(auto& key: groupMaskTypes){
            BlockType type = predefinedBlocks[key];
            if(!type.solid || type.billboard) continue;
            //std::cout << "Solving plane: " << i << std::endl;
            //for(int j = 0;j < 64;j++) std::cout << std::bitset<64>(planes[i][j]) << std::endl;

            processFaces(greedyMeshPlane(planesXforward [static_cast<size_t>(key)], size), X, true , type, solidMesh.get(), worldX, worldY, worldZ, z, scale);
            processFaces(greedyMeshPlane(planesXbackward[static_cast<size_t>(key)], size), X, false, type, solidMesh.get(), worldX, worldY, worldZ, z, scale);

            processFaces(greedyMeshPlane(planesYforward [static_cast<size_t>(key)], size), Y, true , type, solidMesh.get(), worldX, worldY, worldZ, z, scale);
            processFaces(greedyMeshPlane(planesYbackward[static_cast<size_t>(key)], size), Y, false, type, solidMesh.get(), worldX, worldY, worldZ, z, scale);

            processFaces(greedyMeshPlane(planesZforward [static_cast<size_t>(key)], size), Z, true , type, solidMesh.get(), worldX, worldY, worldZ, z, scale);
            processFaces(greedyMeshPlane(planesZbackward[static_cast<size_t>(key)], size), Z, false, type, solidMesh.get(), worldX, worldY, worldZ, z, scale);
        }
    }
    
    /*
        Mesh bilboards
    */

    for(auto& key: groupMaskTypes){
        auto& mask = group->getMask(key);

        BlockType type = predefinedBlocks[mask.getBlock().type];
        if(!type.billboard) continue;

        for(int x = 0;x < size;x++) for(int y = 0;y < size;y++) for(int z = 0;z < size;z++){
            if(!mask.get(x,y,z)) continue;

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
                static_cast<float>(type.textures[0]), // Texture is the first one
                true,
                1,1
            );
            solidMesh->addQuadFaceGreedy(
                vertices1.data(),
                normal,
                occlusion,
                static_cast<float>(type.textures[0]), // Texture is the first one
                false,
                1,1
            );

            solidMesh->addQuadFaceGreedy(
                vertices2.data(),
                normal,
                occlusion,
                static_cast<float>(type.textures[0]), // Texture is the first one
                true,
                1,1
            );
            solidMesh->addQuadFaceGreedy(
                vertices2.data(),
                normal,
                occlusion,
                static_cast<float>(type.textures[0]), // Texture is the first one
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
    
    std::unordered_set<BlockTypes> agregateTypesX = mergeMaskKeys(groupMaskTypes,nextXGroup->getMaskTypes());
    std::unordered_set<BlockTypes> agregateTypesY = mergeMaskKeys(groupMaskTypes,nextYGroup->getMaskTypes());
    std::unordered_set<BlockTypes> agregateTypesZ = mergeMaskKeys(groupMaskTypes,nextZGroup->getMaskTypes());

    std::unordered_set<BlockTypes> fullAgregate;
    fullAgregate.insert(agregateTypesX.begin(),agregateTypesX.end());
    fullAgregate.insert(agregateTypesY.begin(),agregateTypesY.end());
    fullAgregate.insert(agregateTypesZ.begin(),agregateTypesZ.end());

    for(int y = 0;y < size;y++){
        for(auto& key: agregateTypesX){
            const uint64_t localMaskRow = group->hasMask(key)     ? group->getMask(key).getRotatedAt(0,y) : 0ULL;
            const uint64_t otherMaskRow = nextXGroup->hasMask(key) ? nextXGroup->getMask(key).getRotatedAt(size - 1,y)   : 0ULL;
            
            uint64_t allFacesX =  (localMaskRow | otherMaskRow) & (group->getSolidMask().getRotatedAt(0,y) ^ nextXSolid.getRotatedAt(size - 1,y));

            planesXforward[ (size_t)key][y] =  group->getSolidMask().getRotatedAt(0,y) & allFacesX;
            planesXbackward[(size_t)key][y] =  nextXSolid.getRotatedAt(size - 1,y) & allFacesX;
        }

        for(auto& key: agregateTypesY){
            const uint64_t localMaskRow = group->hasMask(key)     ? group->getMask(key).getAt(y,0) : 0ULL;
            const uint64_t otherMaskRow = nextYGroup->hasMask(key) ? nextYGroup->getMask(key).getAt(y,size - 1)   : 0ULL;
            
            uint64_t allFacesY =  (localMaskRow | otherMaskRow) & (group->getSolidMask().getAt(y,0) ^ nextYSolid.getAt(y,size - 1));

            planesYforward[ (size_t) key][y] = group->getSolidMask().getAt(y,0) & allFacesY;
            planesYbackward[(size_t) key][y] = nextYSolid.getAt(y,size - 1) & allFacesY;
        }

        for(auto& key: agregateTypesZ){
            const uint64_t localMaskRow = group->hasMask(key)     ? group->getMask(key).getAt(0,y) : 0ULL;
            const uint64_t otherMaskRow = nextZGroup->hasMask(key) ? nextZGroup->getMask(key).getAt(size - 1,y)   : 0ULL;
            

            uint64_t allFacesZ =  (localMaskRow | otherMaskRow) & (group->getSolidMask().getAt(0,y) ^ nextZSolid.getAt(size - 1,y));
                
            planesZforward[ (size_t) key][y] =  group->getSolidMask().getAt(0,y) & allFacesZ;
            planesZbackward[(size_t) key][y] =  nextZSolid.getAt(size - 1,y) & allFacesZ;
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

    return solidMesh;
}

void Chunk::generateMeshes(){
    //std::cout << "Generating mesh for: " << getMainGroup()->size() << " " << isMainGroupOfSize(64) << " " << isMainGroupOfSize(32) << std::endl;
    solidMesh = generateChunkMesh(world, worldPosition, currentGroup.get());
}

static inline bool isOnOrForwardPlane(const Plane& plane, glm::vec3 center){
    // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
    const float x = CHUNK_SIZE / 2;

    const float r = 
        std::abs(plane.normal.x) * (x) +
        std::abs(plane.normal.y) * (x) +
        std::abs(plane.normal.z) * (x);

    return -r <= plane.getSignedDistanceToPlane(center);
}

bool Chunk::isOnFrustum(PerspectiveCamera& camera) const {
    //Get global scale thanks to our transform
    Frustum& frustum = camera.getFrustum();
    glm::vec3 position = glm::vec3(this->worldPosition.x * CHUNK_SIZE, this->worldPosition.y * CHUNK_SIZE, this->worldPosition.z * CHUNK_SIZE);

    return isOnOrForwardPlane(frustum.leftFace, position) &&
        isOnOrForwardPlane(frustum.rightFace, position) &&
        isOnOrForwardPlane(frustum.topFace, position) &&
        isOnOrForwardPlane(frustum.bottomFace, position) &&
        isOnOrForwardPlane(frustum.nearFace, position) &&
        isOnOrForwardPlane(frustum.farFace, position);
};