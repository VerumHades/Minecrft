#include <game/chunk.hpp>

std::mutex predefinedBlockMutex;
std::unordered_map<BlockTypes, BlockType> predefinedBlocks = {
    {BlockTypes::Air            , BlockType(true , true)}, // Air
    {BlockTypes::Grass          , BlockType(false, false, false, {0, 2, 1, 1, 1, 1})}, // Grass
    {BlockTypes::Dirt           , BlockType(false, false, true, {2})}, // Dirt
    {BlockTypes::Stone          , BlockType(false, false, true, {3})}, // Stone
    {BlockTypes::LeafBlock      , BlockType(false, false, true, {6})}, // Leaf Block
    {BlockTypes::OakLog         , BlockType(false, false, false, {4, 4, 5, 5, 5, 5})}, // Oak Log
    {BlockTypes::BirchLeafBlock , BlockType(false, false, true, {7})}, // Birch Leaf Block
    {BlockTypes::BirchLog       , BlockType(false, false, false, {9, 9, 8, 8, 8, 8})}, // Birch Log
    {BlockTypes::BlueWool       , BlockType(false, false, true, {10})}, // Blue Wool
    {BlockTypes::Sand           , BlockType(false, false, true, {11})} // Sand
}; 

#define BLT(name) case BlockTypes::name: return #name;
std::string getBlockTypeName(BlockTypes type){
    switch(type){
        BLT(Air)
        BLT(Grass) 
        BLT(Dirt)
        BLT(Stone)
        BLT(LeafBlock)
        BLT(OakLog)
        BLT(BirchLeafBlock)
        BLT(BirchLog)
        BLT(BlueWool)
        BLT(Sand)
        default: return "Unknown?";
    }
}
#undef BLT

static inline void fillChunkLevel(Chunk& chunk, uint32_t y, Block value){
    for(int x = 0; x < CHUNK_SIZE;x++) for(int z = 0;z < CHUNK_SIZE;z++){
        chunk.setBlock(x,y,z, value);
    }
}

Chunk::Chunk(World& world, const glm::vec3& pos): world(world), worldPosition(pos) {
    
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
    
    uint64_f checkMask = (1_uint64 << (63 - x));
    
    for(auto& [key,mask]: masks){
        uint64_f row = mask.segments[z][y];

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

    uint64_f currentMask = (1_uint64 << (63 - x));
    
    if(value.type != BlockTypes::Air){
        if(masks.count(value.type) == 0){
            masks[value.type] = {};
            masks[value.type].block = value;
        }

        masks[value.type].set(x,y,z);
    }

    for(auto& [key,mask]: masks){
        uint64_f row = mask.segments[z][y];

        if(!(row & currentMask)) continue;
        if(mask.block.type == value.type) continue;

        mask.reset(x,y,z);
        break;
    }

    if(getBlockType(&value).untextured) solidMask.reset(x,y,z);
    else solidMask.set(x,y,z);

    return true;
}

/*
    Generate greedy meshed faces from a plane of bits
*/
std::vector<Face> greedyMeshPlane64(Plane64 rows){
    std::vector<Face> out = {};
    int currentRow = 0;
    
    while(currentRow < 64){
        uint64_f row = rows[currentRow];
        /*
            0b00001101

            'start' is 4
        */    
        uint8_t start = count_leading_zeros(row); // Find the first
        if(start == 64){
            currentRow++;
            continue;
        }
        /*
            2. 0b11010000 shift by 4
            1. 0b00101111 negate

            'width' is 2
        */    
        row <<= start; // Shift so the faces start
        uint8_t width = count_leading_zeros(~row); // Calculate width (negated counts '1') 
        row >>= start; // Return to original position

        /*
                Create a mask of all ones
                1. 0b11111111
                2. 0b00000011 shift beyond the face (start + width)
                3. 0b11111100 negate

                    0b11111100 & 0b00001101

                4. 0b00001100 AND with the row to create the faces mask
        */
        uint64_f mask = ~0_uint64;

        //  Shifting by 64 is undefined behaviour for some reason ¯\_(ツ)_/¯
        if((start + width) != 64) mask = ~(mask >> (start + width));
        mask &= row;

        int height = 0; 
        while(currentRow + height < 64 && (mask & rows[currentRow + height]) == mask){
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

static inline std::unordered_set<BlockTypes> mergeMaskKeys(const std::unordered_map<BlockTypes, ChunkMask<uint64_f>>& a, const std::unordered_map<BlockTypes, ChunkMask<uint64_f>>& b){
    std::unordered_set<BlockTypes> out;
    for(auto& [key,_]: a) out.emplace(key);
    for(auto& [key,_]: b) out.emplace(key);
    return out;
}

std::vector<Face> greedyMeshDualPlane64(const Plane64& a, const Plane64& b){
    std::vector<Face> vA = greedyMeshPlane64(a);
    std::vector<Face> vB = greedyMeshPlane64(b);
    vA.insert(vA.end(), vB.begin(), vB.end());
    return vA;
}

void Chunk::generateMeshes(){
    this->solidMesh = std::make_unique<Mesh>();
    //this->solidMesh->setVertexFormat(VertexFormat({3,1,2,1,1}));  // Unused

    float worldX = getWorldPosition().x * CHUNK_SIZE;
    float worldY = getWorldPosition().y * CHUNK_SIZE;
    float worldZ = getWorldPosition().z * CHUNK_SIZE;

    for(int z = 0; z < CHUNK_SIZE - 1;z++){
        PlaneArray64 planesXforward = {0};
        PlaneArray64 planesXbackward = {0};

        PlaneArray64 planesYforward = {0};
        PlaneArray64 planesYbackward = {0};

        PlaneArray64 planesZforward = {0};
        PlaneArray64 planesZbackward = {0};
        
        for(int y = 0;y < CHUNK_SIZE;y++){
            for(auto& [key,mask]: masks){
                uint64_f allFacesX = (mask.segmentsRotated[z][y] | mask.segmentsRotated[z + 1][y]) & (solidMask.segmentsRotated[z][y] ^ solidMask.segmentsRotated[z + 1][y]);
                planesXforward[ (size_t) mask.block.type][y] = solidMask.segmentsRotated[z][y] & allFacesX;
                planesXbackward[(size_t) mask.block.type][y] = solidMask.segmentsRotated[z + 1][y] & allFacesX;

                uint64_f allFacesY = (mask.segments[y][z] | mask.segments[y][z + 1]) & (solidMask.segments[y][z] ^ solidMask.segments[y][z + 1]);
                planesYforward[ (size_t) mask.block.type][y] = solidMask.segments[y][z] & allFacesY;
                planesYbackward[(size_t) mask.block.type][y] = solidMask.segments[y][z + 1] & allFacesY;

                uint64_f allFacesZ = (mask.segments[z][y] | mask.segments[z + 1][y]) & (solidMask.segments[z][y] ^ solidMask.segments[z + 1][y]);
                planesZforward[ (size_t) mask.block.type][y] = solidMask.segments[z][y] & allFacesZ;
                planesZbackward[(size_t) mask.block.type][y] = solidMask.segments[z + 1][y] & allFacesZ;
            }
        }

        for(auto& [key,mask]: masks){
            BlockType type = predefinedBlocks[mask.block.type];
            if(type.untextured) continue;
            //std::cout << "Solving plane: " << i << std::endl;
            //for(int j = 0;j < 64;j++) std::cout << std::bitset<64>(planes[i][j]) << std::endl;
            
            std::vector<Face> facesX = greedyMeshDualPlane64(planesXforward[(size_t) mask.block.type], planesXbackward[(size_t) mask.block.type]);
            std::vector<Face> facesY = greedyMeshDualPlane64(planesYforward[(size_t) mask.block.type], planesYbackward[(size_t) mask.block.type]);
            std::vector<Face> facesZ = greedyMeshDualPlane64(planesZforward[(size_t) mask.block.type], planesZbackward[(size_t) mask.block.type]);


            float occlusion[4] = {0,0,0,0};

            for(auto& face: facesX){
                std::array<glm::vec3, 4> vertices = {
                    glm::vec3(worldX + z + 1, worldY + face.y + face.height, worldZ + face.x             ),
                    glm::vec3(worldX + z + 1, worldY + face.y + face.height, worldZ + face.x + face.width),
                    glm::vec3(worldX + z + 1, worldY + face.y              , worldZ + face.x + face.width),
                    glm::vec3(worldX + z + 1, worldY + face.y              , worldZ + face.x             )
                };

                int direction = (solidMask.segmentsRotated[z + 1][face.y] >> (63 - face.x)) & 1_uint64;
                int texture = type.repeatTexture ? type.textures[0] : type.textures[4 + direction];

                int normal = direction ? 2 : 3;

                solidMesh->addQuadFaceGreedy(
                    vertices.data(),
                    normal,
                    occlusion,
                    static_cast<float>(texture),
                    !direction,
                    face.width,
                    face.height
                );
            }

            for(auto& face: facesY){
                std::array<glm::vec3, 4> vertices = {
                    glm::vec3(worldX + face.x             , worldY + z + 1, worldZ + face.y              ),
                    glm::vec3(worldX + face.x + face.width, worldY + z + 1, worldZ + face.y              ),
                    glm::vec3(worldX + face.x + face.width, worldY + z + 1, worldZ + face.y + face.height),
                    glm::vec3(worldX + face.x             , worldY + z + 1, worldZ + face.y + face.height)
                };

                int direction = (solidMask.segments[face.y][z + 1] >> (63 - face.x)) & 1_uint64;
                int texture = type.repeatTexture ? type.textures[0] : type.textures[direction];

                int normal = direction ? 0 : 1;

                solidMesh->addQuadFaceGreedy(
                    vertices.data(),
                    normal,
                    occlusion,
                    static_cast<float>(texture),
                    direction,
                    face.width,
                    face.height
                );
            }

            for(auto& face: facesZ){
                std::array<glm::vec3, 4> vertices = {
                    glm::vec3(worldX + face.x              , worldY + face.y + face.height, worldZ + z + 1),
                    glm::vec3(worldX + face.x + face.width , worldY + face.y + face.height, worldZ + z + 1),
                    glm::vec3(worldX + face.x + face.width , worldY + face.y              , worldZ + z + 1),
                    glm::vec3(worldX + face.x              , worldY + face.y              , worldZ + z + 1)
                };

                int direction = (solidMask.segments[z + 1][face.y] >> (63 - face.x)) & 1_uint64;
                int texture = type.repeatTexture ? type.textures[0] : type.textures[2 + direction];

                int normal = direction ? 4 : 5;

                solidMesh->addQuadFaceGreedy(
                    vertices.data(),
                    normal,
                    occlusion,
                    static_cast<float>(texture),
                    direction,
                    face.width,
                    face.height
                );
            }
        }
    }

    PlaneArray64 planesXforward = {0};
    PlaneArray64 planesXbackward = {0};

    PlaneArray64 planesYforward = {0};
    PlaneArray64 planesYbackward = {0};

    PlaneArray64 planesZforward = {0};
    PlaneArray64 planesZbackward = {0};
    

    Chunk* nextX = world.getChunk(worldPosition.x - 1,worldPosition.y,worldPosition.z);
    Chunk* nextY = world.getChunk(worldPosition.x,worldPosition.y - 1,worldPosition.z);
    Chunk* nextZ = world.getChunk(worldPosition.x,worldPosition.y,worldPosition.z - 1);

    if(!nextX || !nextY || !nextZ){
        std::cout << "Mesh generating when chunks are missing?" << std::endl;
        return;
    }
    const ChunkMask<uint64_f>& nextXSolid = nextX->getSolidMask();
    const ChunkMask<uint64_f>& nextYSolid = nextY->getSolidMask();
    const ChunkMask<uint64_f>& nextZSolid = nextZ->getSolidMask();

    auto& nextXmasks = nextX->getMasks();
    auto& nextYmasks = nextY->getMasks();
    auto& nextZmasks = nextZ->getMasks();
    
    std::unordered_set<BlockTypes> agregateTypesX = mergeMaskKeys(masks,nextXmasks);
    std::unordered_set<BlockTypes> agregateTypesY = mergeMaskKeys(masks,nextYmasks);
    std::unordered_set<BlockTypes> agregateTypesZ = mergeMaskKeys(masks,nextZmasks);

    std::unordered_set<BlockTypes> fullAgregate;
    fullAgregate.insert(agregateTypesX.begin(),agregateTypesX.end());
    fullAgregate.insert(agregateTypesY.begin(),agregateTypesY.end());
    fullAgregate.insert(agregateTypesZ.begin(),agregateTypesZ.end());

    for(int y = 0;y < CHUNK_SIZE;y++){
        for(auto& key: agregateTypesX){
            const uint64_t localMaskRow = masks.count(key)      != 0 ? masks.at(key)     .segmentsRotated[0] [y] : 0_uint64;
            const uint64_t otherMaskRow = nextXmasks.count(key) != 0 ? nextXmasks.at(key).segmentsRotated[63][y] : 0_uint64;
            
            uint64_f allFacesX =  (localMaskRow | otherMaskRow) & (solidMask.segmentsRotated[0][y] ^ nextXSolid.segmentsRotated[63][y]);

            planesXforward[ (size_t)key][y] =  solidMask.segmentsRotated[0][y] & allFacesX;
            planesXbackward[(size_t)key][y] =  nextXSolid.segmentsRotated[63][y] & allFacesX;
        }

        for(auto& key: agregateTypesY){
            const uint64_t localMaskRow = masks.count(key)      != 0 ? masks.at(key)     .segments[y] [0] : 0_uint64;
            const uint64_t otherMaskRow = nextYmasks.count(key) != 0 ? nextYmasks.at(key).segments[y][63] : 0_uint64;
            
            uint64_f allFacesY =  (localMaskRow | otherMaskRow) & (solidMask.segments[y][0] ^ nextYSolid.segments[y][63]);

            planesYforward[ (size_t) key][y] = solidMask.segments[y][0] & allFacesY;
            planesYbackward[(size_t) key][y] = nextYSolid.segments[y][63] & allFacesY;
        }

        for(auto& key: agregateTypesZ){
            const uint64_t localMaskRow = masks.count(key)      != 0 ? masks.at(key)     .segments[0] [y] : 0_uint64;
            const uint64_t otherMaskRow = nextZmasks.count(key) != 0 ? nextZmasks.at(key).segments[63][y] : 0_uint64;
            

            uint64_f allFacesZ =  (localMaskRow | otherMaskRow) & (solidMask.segments[0][y] ^ nextZSolid.segments[63][y]);
                
            planesZforward[ (size_t) key][y] =  solidMask.segments[0][y] & allFacesZ;
            planesZbackward[(size_t) key][y] =  nextZSolid.segments[63][y] & allFacesZ;
        }
    }

    for(auto& key: fullAgregate){
        BlockType type = predefinedBlocks[key];
        if(type.untextured) continue;
        
        std::vector<Face> facesX = greedyMeshDualPlane64(planesXforward[(size_t) key], planesXbackward[(size_t) key]);
        std::vector<Face> facesY = greedyMeshDualPlane64(planesYforward[(size_t) key], planesYbackward[(size_t) key]);
        std::vector<Face> facesZ = greedyMeshDualPlane64(planesZforward[(size_t) key], planesZbackward[(size_t) key]);

        float occlusion[4] = {0,0,0,0};

        for(auto& face: facesX){
            std::array<glm::vec3, 4> vertices = {
                glm::vec3(worldX, worldY + face.y + face.height, worldZ + face.x             ),
                glm::vec3(worldX, worldY + face.y + face.height, worldZ + face.x + face.width),
                glm::vec3(worldX, worldY + face.y              , worldZ + face.x + face.width),
                glm::vec3(worldX, worldY + face.y              , worldZ + face.x             )
            };

            int direction = (solidMask.segmentsRotated[0][face.y] >> (63 - face.x)) & 1_uint64;
            int texture = type.repeatTexture ? type.textures[0] : type.textures[4];

            int normal = direction ? 2 : 3;

            solidMesh->addQuadFaceGreedy(
                vertices.data(),
                normal,
                occlusion,
                static_cast<float>(texture),
                !direction,
                face.width,
                face.height
            );
        }

        for(auto& face: facesY){
            std::array<glm::vec3, 4> vertices = {
                glm::vec3(worldX + face.x             , worldY, worldZ + face.y              ),
                glm::vec3(worldX + face.x + face.width, worldY, worldZ + face.y              ),
                glm::vec3(worldX + face.x + face.width, worldY, worldZ + face.y + face.height),
                glm::vec3(worldX + face.x             , worldY, worldZ + face.y + face.height)
            };

            int direction = ((solidMask.segments[face.y][0] >> (63 - face.x)) & 1_uint64);

            int normal = direction ? 0 : 1;

            solidMesh->addQuadFaceGreedy(
                vertices.data(),
                normal,
                occlusion,
                static_cast<float>(type.textures[0]),
                direction,
                face.width,
                face.height
            );
        }

        for(auto& face: facesZ){
            std::array<glm::vec3, 4> vertices = {
                glm::vec3(worldX + face.x              , worldY + face.y + face.height, worldZ),
                glm::vec3(worldX + face.x + face.width , worldY + face.y + face.height, worldZ),
                glm::vec3(worldX + face.x + face.width , worldY + face.y              , worldZ),
                glm::vec3(worldX + face.x              , worldY + face.y              , worldZ)
            };
            
            int direction = (solidMask.segments[0][face.y] >> (63 - face.x)) & 1_uint64;
            int texture = type.repeatTexture ? type.textures[0] : type.textures[2];

            int normal = direction ? 4 : 5;

            solidMesh->addQuadFaceGreedy(
                vertices.data(),
                normal,
                occlusion,
                static_cast<float>(texture),
                direction,
                face.width,
                face.height
            );
        }
    }

    isEmpty = this->solidMesh.get()->getVertices().size() == 0;
    //std::cout << "Vertices:" << this->solidMesh.get()->getIndices().size() << std::endl;
}


static inline bool isOnOrForwardPlane(const Plane& plane, glm::vec3 center){
    // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
    const float x = CHUNK_SIZE;

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