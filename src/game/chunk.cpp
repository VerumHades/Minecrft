#include <game/chunk.hpp>

static inline void fillChunkLevel(Chunk& chunk, uint32_t y, Block value){
    for(int x = 0; x < CHUNK_SIZE;x++) for(int z = 0;z < CHUNK_SIZE;z++){
        chunk.setBlock(x,y,z, value);
    }
}

Chunk::Chunk(World& world, const glm::vec3& pos): world(world), worldPosition(pos) {
    currentGroup = std::make_unique<ChunkMaskGroup<64>>();
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
    auto mainGroup = getMainGroupAs<64>();
    
    for(auto& [key,mask]: mainGroup->masks){
        uint64_t row = mask.segments[z][y];

        if(row & checkMask){
            return &mask.block;
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
    auto mainGroup = getMainGroupAs<64>();

    if(value.type != BlockTypes::Air){
        if(mainGroup->masks.count(value.type) == 0){
            mainGroup->masks[value.type] = {};
            mainGroup->masks[value.type].block = value;
        }

        mainGroup->masks[value.type].set(x,y,z);
    }

    for(auto& [key,mask]: mainGroup->masks){
        uint64_t row = mask.segments[z][y];

        if(!(row & currentMask)) continue;
        if(mask.block.type == value.type) continue;

        mask.reset(x,y,z);
        break;
    }

    if(getBlockType(&value).untextured) mainGroup->solidMask.reset(x,y,z);
    else mainGroup->solidMask.set(x,y,z);

    return true;
}

void Chunk::updateMesh(){
    reloadMesh = true;
    generatedEmptyMesh = false;
}

void Chunk::generateMesh(MultiChunkBuffer& buffer, ThreadPool& pool){
    if(
        !world.getChunk(worldPosition.x - 1,worldPosition.y,worldPosition.z) ||
        !world.getChunk(worldPosition.x,worldPosition.y - 1,worldPosition.z) ||
        !world.getChunk(worldPosition.x,worldPosition.y,worldPosition.z - 1) 
    ) return;

    if(
        !meshGenerated && !meshGenerating && !pendingUpload && !generatedEmptyMesh && 
        ((!buffer.isChunkLoaded(worldPosition) && !isEmpty()) || reloadMesh) // If chunk isnt loaded at all
    ){
        bool success = pool.deploy([this](){
            //auto start = std::chrono::high_resolution_clock::now();

            generateMeshes();

            //End time point
            //auto end = std::chrono::high_resolution_clock::now();

            //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            //std::cout << "Execution time: " << duration << " microseconds" << std::endl;

            meshGenerating = false;
            meshGenerated = true;
            pendingUpload = true;
        });
        if(!success) return;

        meshGenerating = true;
        //generateChunkMeshThread(chunk);
        return;
    } 

    if(meshGenerated){
        if(solidMesh->getVertices().size() == 0){
            generatedEmptyMesh = true;
        }
        else if(reloadMesh){
            buffer.swapChunkMesh(*solidMesh, getWorldPosition());
            reloadMesh = false;
        }
        else buffer.addChunkMesh(*solidMesh, getWorldPosition());
        
        solidMesh = nullptr;
        meshGenerated = false;
        pendingUpload = false;
        return;
    }

    /*if(
        buffer.isChunkLoaded(glm::vec3(worldPosition.x + 1,worldPosition.y,worldPosition.z)) &&
        buffer.isChunkLoaded(glm::vec3(worldPosition.x,worldPosition.y + 1,worldPosition.z)) &&
        buffer.isChunkLoaded(glm::vec3(worldPosition.x,worldPosition.y,worldPosition.z + 1)) &&
        getMainGroup()
    ){
        setMainGroup(nullptr);
    }*/

    /*if(!buffersLoaded && !buffersInQue && meshGenerated){
        buffersInQue = true;
        this->bufferLoadQue.push(getWorldPosition());
        //printf("Vertices:%i Indices:%i\n", solidMesh->vertices_count, solidMesh->indices_count)
        updated = true;
        if(isDrawn) return chunk;
    }*/
}

void Chunk::syncGenerateMesh(MultiChunkBuffer& buffer){
    generateMeshes();

    if(solidMesh->getVertices().size() == 0){
        generatedEmptyMesh = true;
    }
    else if(reloadMesh){
        buffer.swapChunkMesh(*solidMesh, getWorldPosition());
        reloadMesh = false;
    }
    else buffer.addChunkMesh(*solidMesh, getWorldPosition());
    
    solidMesh = nullptr;
}

/*
    Generate greedy meshed faces from a plane of bits
*/
template <int size>
std::vector<Face> greedyMeshPlane(std::array<uint<size>, size> rows){
    std::vector<Face> out = {};

    int currentRow = 0;
    
    while(currentRow < size){
        uint<size> row = rows[currentRow];
        /*
            0b00001101

            'start' is 4
        */    
        uint8_t start = count_leading_zeros<uint<size>>(row); // Find the first
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
        uint8_t width = count_leading_zeros<uint<size>>(~row); // Calculate width (negated counts '1') 
        row >>= start; // Return to original position

        /*
                Create a mask of all ones
                1. 0b11111111
                2. 0b00000011 shift beyond the face (start + width)
                3. 0b11111100 negate

                    0b11111100 & 0b00001101

                4. 0b00001100 AND with the row to create the faces mask
        */
        uint<size> mask = ~0ULL;

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

template <int size>
static inline std::unordered_set<BlockTypes> mergeMaskKeys(const std::unordered_map<BlockTypes, ChunkMask<size>>& a, const std::unordered_map<BlockTypes, ChunkMask<size>>& b){
    std::unordered_set<BlockTypes> out;
    for(auto& [key,_]: a) out.emplace(key);
    for(auto& [key,_]: b) out.emplace(key);
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

template <int size>
std::unique_ptr<Mesh> generateChunkMesh(World& world, glm::ivec3 worldPosition, ChunkMaskGroup<size>* group){
    auto solidMesh = std::make_unique<Mesh>();
    //this->solidMesh->setVertexFormat(VertexFormat({3,1,2,1,1}));  // Unused
    float worldX = worldPosition.x * CHUNK_SIZE;
    float worldY = worldPosition.y * CHUNK_SIZE;
    float worldZ = worldPosition.z * CHUNK_SIZE;

    float scale = CHUNK_SIZE / size;

    for(int z = 0; z < size - 1;z++){
        BlockBitPlanes<size> planesXforward = {0};
        BlockBitPlanes<size> planesXbackward = {0};

        BlockBitPlanes<size> planesYforward = {0};
        BlockBitPlanes<size> planesYbackward = {0};

        BlockBitPlanes<size> planesZforward = {0};
        BlockBitPlanes<size> planesZbackward = {0};
        
        for(int y = 0;y < size;y++){
            for(auto& [key,mask]: group->masks){
                uint<size> allFacesX = (mask.segmentsRotated[z][y] | mask.segmentsRotated[z + 1][y]) & (group->solidMask.segmentsRotated[z][y] ^ group->solidMask.segmentsRotated[z + 1][y]);
                planesXforward[ (size_t) mask.block.type][y] = group->solidMask.segmentsRotated[z][y] & allFacesX;
                planesXbackward[(size_t) mask.block.type][y] = group->solidMask.segmentsRotated[z + 1][y] & allFacesX;

                uint<size> allFacesY = (mask.segments[y][z] | mask.segments[y][z + 1]) & (group->solidMask.segments[y][z] ^ group->solidMask.segments[y][z + 1]);
                planesYforward[ (size_t) mask.block.type][y] = group->solidMask.segments[y][z] & allFacesY;
                planesYbackward[(size_t) mask.block.type][y] = group->solidMask.segments[y][z + 1] & allFacesY;

                uint<size> allFacesZ = (mask.segments[z][y] | mask.segments[z + 1][y]) & (group->solidMask.segments[z][y] ^ group->solidMask.segments[z + 1][y]);
                planesZforward[ (size_t) mask.block.type][y] = group->solidMask.segments[z][y] & allFacesZ;
                planesZbackward[(size_t) mask.block.type][y] = group->solidMask.segments[z + 1][y] & allFacesZ;
            }
        }

        for(auto& [key,mask]: group->masks){
            BlockType type = predefinedBlocks[mask.block.type];
            if(type.untextured) continue;
            //std::cout << "Solving plane: " << i << std::endl;
            //for(int j = 0;j < 64;j++) std::cout << std::bitset<64>(planes[i][j]) << std::endl;

            processFaces(greedyMeshPlane<size>(planesXforward [static_cast<size_t>(mask.block.type)]), X, true , type, solidMesh.get(), worldX, worldY, worldZ, z, scale);
            processFaces(greedyMeshPlane<size>(planesXbackward[static_cast<size_t>(mask.block.type)]), X, false, type, solidMesh.get(), worldX, worldY, worldZ, z, scale);

            processFaces(greedyMeshPlane<size>(planesYforward [static_cast<size_t>(mask.block.type)]), Y, true , type, solidMesh.get(), worldX, worldY, worldZ, z, scale);
            processFaces(greedyMeshPlane<size>(planesYbackward[static_cast<size_t>(mask.block.type)]), Y, false, type, solidMesh.get(), worldX, worldY, worldZ, z, scale);

            processFaces(greedyMeshPlane<size>(planesZforward [static_cast<size_t>(mask.block.type)]), Z, true , type, solidMesh.get(), worldX, worldY, worldZ, z, scale);
            processFaces(greedyMeshPlane<size>(planesZbackward[static_cast<size_t>(mask.block.type)]), Z, false, type, solidMesh.get(), worldX, worldY, worldZ, z, scale);
        }
    }

    BlockBitPlanes<size> planesXforward = {0};
    BlockBitPlanes<size> planesXbackward = {0};

    BlockBitPlanes<size> planesYforward = {0};
    BlockBitPlanes<size> planesYbackward = {0};

    BlockBitPlanes<size> planesZforward = {0};
    BlockBitPlanes<size> planesZbackward = {0};
    

    Chunk* nextX = world.getChunk(worldPosition.x - 1,worldPosition.y,worldPosition.z);
    Chunk* nextY = world.getChunk(worldPosition.x,worldPosition.y - 1,worldPosition.z);
    Chunk* nextZ = world.getChunk(worldPosition.x,worldPosition.y,worldPosition.z - 1);

    if(!nextX || !nextY || !nextZ){
        std::cout << "Mesh generating when chunks are missing?" << std::endl;
        return solidMesh;
    }

    if(!nextX->isMainGroupOfSize(size) || !nextY->isMainGroupOfSize(size) || !nextZ->isMainGroupOfSize(size)){
        //std::cout << "Mesh generating when wrong sizes?" << std::endl;
        return solidMesh;
    }

    const ChunkMask<size>& nextXSolid = nextX->getMainGroupAs<size>()->solidMask;
    const ChunkMask<size>& nextYSolid = nextY->getMainGroupAs<size>()->solidMask;
    const ChunkMask<size>& nextZSolid = nextZ->getMainGroupAs<size>()->solidMask;

    auto& nextXmasks = nextX->getMainGroupAs<size>()->masks;
    auto& nextYmasks = nextY->getMainGroupAs<size>()->masks;
    auto& nextZmasks = nextZ->getMainGroupAs<size>()->masks;
    
    std::unordered_set<BlockTypes> agregateTypesX = mergeMaskKeys(group->masks,nextXmasks);
    std::unordered_set<BlockTypes> agregateTypesY = mergeMaskKeys(group->masks,nextYmasks);
    std::unordered_set<BlockTypes> agregateTypesZ = mergeMaskKeys(group->masks,nextZmasks);

    std::unordered_set<BlockTypes> fullAgregate;
    fullAgregate.insert(agregateTypesX.begin(),agregateTypesX.end());
    fullAgregate.insert(agregateTypesY.begin(),agregateTypesY.end());
    fullAgregate.insert(agregateTypesZ.begin(),agregateTypesZ.end());

    for(int y = 0;y < size;y++){
        for(auto& key: agregateTypesX){
            const uint64_t localMaskRow = group->masks.count(key) != 0 ? group->masks.at(key).segmentsRotated[0] [y] : 0ULL;
            const uint64_t otherMaskRow = nextXmasks.count(key)   != 0 ? nextXmasks.at(key).segmentsRotated[size - 1][y]   : 0ULL;
            
            uint64_t allFacesX =  (localMaskRow | otherMaskRow) & (group->solidMask.segmentsRotated[0][y] ^ nextXSolid.segmentsRotated[size - 1][y]);

            planesXforward[ (size_t)key][y] =  group->solidMask.segmentsRotated[0][y] & allFacesX;
            planesXbackward[(size_t)key][y] =  nextXSolid.segmentsRotated[size - 1][y] & allFacesX;
        }

        for(auto& key: agregateTypesY){
            const uint64_t localMaskRow = group->masks.count(key) != 0 ? group->masks.at(key).segments[y] [0] : 0ULL;
            const uint64_t otherMaskRow = nextYmasks.count(key)   != 0 ? nextYmasks.at(key).segments[y][size - 1]   : 0ULL;
            
            uint64_t allFacesY =  (localMaskRow | otherMaskRow) & (group->solidMask.segments[y][0] ^ nextYSolid.segments[y][size - 1]);

            planesYforward[ (size_t) key][y] = group->solidMask.segments[y][0] & allFacesY;
            planesYbackward[(size_t) key][y] = nextYSolid.segments[y][size - 1] & allFacesY;
        }

        for(auto& key: agregateTypesZ){
            const uint64_t localMaskRow = group->masks.count(key) != 0 ? group->masks.at(key).segments[0] [y] : 0ULL;
            const uint64_t otherMaskRow = nextZmasks.count(key)   != 0 ? nextZmasks.at(key).segments[size - 1][y]   : 0ULL;
            

            uint64_t allFacesZ =  (localMaskRow | otherMaskRow) & (group->solidMask.segments[0][y] ^ nextZSolid.segments[size - 1][y]);
                
            planesZforward[ (size_t) key][y] =  group->solidMask.segments[0][y] & allFacesZ;
            planesZbackward[(size_t) key][y] =  nextZSolid.segments[size - 1][y] & allFacesZ;
        }
    }

    for(auto& key: fullAgregate){
        BlockType type = predefinedBlocks[key];
        if(type.untextured) continue;

        processFaces(greedyMeshPlane<size>(planesXforward [static_cast<size_t>(key)]), X, false, type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
        processFaces(greedyMeshPlane<size>(planesXbackward[static_cast<size_t>(key)]), X, true , type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);

        processFaces(greedyMeshPlane<size>(planesYforward [static_cast<size_t>(key)]), Y, false, type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
        processFaces(greedyMeshPlane<size>(planesYbackward[static_cast<size_t>(key)]), Y, true , type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);

        processFaces(greedyMeshPlane<size>(planesZforward [static_cast<size_t>(key)]), Z, false, type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
        processFaces(greedyMeshPlane<size>(planesZbackward[static_cast<size_t>(key)]), Z, true , type, solidMesh.get(), worldX, worldY, worldZ, -1, scale);
    }
    //std::cout << "Vertices:" << solidMesh.get()->getIndices().size() << std::endl;

    return solidMesh;
}

void Chunk::generateMeshes(){
    //std::cout << "Generating mesh for: " << getMainGroup()->getSize() << " " << isMainGroupOfSize(64) << " " << isMainGroupOfSize(32) << std::endl;
    if     (isMainGroupOfSize(64)) solidMesh = generateChunkMesh<64>(world, worldPosition, getMainGroupAs<64>());
    else if(isMainGroupOfSize(32)) solidMesh = generateChunkMesh<32>(world, worldPosition, getMainGroupAs<32>());
    else if(isMainGroupOfSize(16)) solidMesh = generateChunkMesh<16>(world, worldPosition, getMainGroupAs<16>());
    else if(isMainGroupOfSize(8))  solidMesh = generateChunkMesh<8> (world, worldPosition, getMainGroupAs<8> ());
    else solidMesh = std::make_unique<Mesh>(); // Fall back to empty;
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