#include <chunk.hpp>

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

void Chunk::regenerateMesh(){
    if(!this->buffersLoaded) return;
    if(!this->isDrawn) return;
    if(this->meshGenerating) return;

    this->meshGenerated = 0;
    this->meshGenerating = 0;
    this->buffersLoaded = 0;
}

#define regenMesh(x,y,z) { \
    Chunk* temp = this->world.getChunk(x, y, z);\
    if(temp) temp->regenerateMesh();\
}
void Chunk::regenerateMesh(glm::vec3 blockCoords){
    this->regenerateMesh();
    if(blockCoords.x == 0)              regenMesh((int) this->worldPosition.x - 1, (int) this->worldPosition.y, (int) this->worldPosition.z);
    if(blockCoords.x == CHUNK_SIZE - 1) regenMesh((int) this->worldPosition.x + 1, (int) this->worldPosition.y, (int) this->worldPosition.z);

    if(blockCoords.y == 0)              regenMesh((int) this->worldPosition.x, (int) this->worldPosition.y - 1, (int) this->worldPosition.z);
    if(blockCoords.y == CHUNK_SIZE - 1) regenMesh((int) this->worldPosition.x, (int) this->worldPosition.y + 1, (int) this->worldPosition.z);

    if(blockCoords.z == 0)              regenMesh((int) this->worldPosition.x, (int) this->worldPosition.y, (int) this->worldPosition.z - 1);
    if(blockCoords.z == CHUNK_SIZE - 1) regenMesh((int) this->worldPosition.x, (int) this->worldPosition.y, (int) this->worldPosition.z + 1);
}
#undef regenMesh


Block* Chunk::getBlock(uint32_t x, uint32_t y, uint32_t z){
    if(x >= CHUNK_SIZE) return nullptr;
    if(y >= CHUNK_SIZE) return nullptr;
    if(z >= CHUNK_SIZE) return nullptr;
    
    uint64_t checkMask = (1ULL << (63 - x));
    
    for(auto& [key,mask]: masks){
        uint64_t row = mask.segments[z][y];

        if(row & checkMask){
            return &mask.block;
        }
    }

    return nullptr;
}

bool Chunk::setBlock(uint32_t x, uint32_t y, uint32_t z, Block value){
    if(x >= CHUNK_SIZE) return false;
    if(y >= CHUNK_SIZE) return false;
    if(z >= CHUNK_SIZE) return false;

    uint64_t currentMask = (1ULL << (63 - x));

    if(masks.count(value.type) == 0) masks[value.type] = {};
    masks[value.type].block = value;
    masks[value.type].set(x,y,z);

    for(auto& [key,mask]: masks){
        uint64_t row = mask.segments[z][y];

        if(!(row & currentMask)) continue;
        if(mask.block.type == value.type) continue;

        mask.reset(x,y,z);
        break;
    }

    if(getBlockType(&value).untextured) solidMask.reset(x,y,z);
    else solidMask.set(x,y,z);

    return true;
}

static inline Block* getWorldBlockFast(Chunk* chunk, int ix, int iy, int iz, int x, int y, int z){
    if(y < 0 || y >= CHUNK_SIZE) return nullptr;

    Block* block = chunk->getBlock(ix, iy, iz);
    if(!block) return chunk->getWorld().getBlock(x, y, z);
    return block;
}

inline int count_leading_zeros(uint64_t x) {
    return std::countl_zero(x);
}

std::vector<Face> greedyMeshPlane64(std::array<uint64_t, 64> rows){
    std::vector<Face> out = {};
    int currentRow = 0, i = 0;
    
    while(currentRow < 64 && i < 50){
        uint64_t row = rows[currentRow];
        /*
            0b00001101

            'start' is 4
        */    
        int start = count_leading_zeros(row); // Find the first
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
        int width = count_leading_zeros(~row); // Calculate width (negated counts '1') 
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

        i++;
    }

    return out;
}

inline uint64_t bit_set(uint64_t number, uint64_t index) {
    return number | ((uint64_t)1 << index);
}

void Chunk::generateMeshes(){
    this->solidMesh = std::make_unique<Mesh>();
    this->solidMesh->setVertexFormat({3,3,2,1,1});     

    for(int z = 0; z < CHUNK_SIZE - 1;z++){
        std::array<std::array<uint64_t, 64>, (size_t) BlockTypes::BLOCK_TYPES_TOTAL> planesX = {0};
        std::array<std::array<uint64_t, 64>, (size_t) BlockTypes::BLOCK_TYPES_TOTAL> planesY = {0};
        std::array<std::array<uint64_t, 64>, (size_t) BlockTypes::BLOCK_TYPES_TOTAL> planesZ = {0};
        
        for(int y = 0;y < CHUNK_SIZE;y++){
            for(auto& [key,mask]: masks){
                planesX[(size_t) mask.block.type][y] = (mask.segmentsRotated[z][y] | mask.segmentsRotated[z + 1][y]) & (solidMask.segmentsRotated[z][y] ^ solidMask.segmentsRotated[z + 1][y]);
                planesY[(size_t) mask.block.type][y] = (mask.segments[y][z] | mask.segments[y][z + 1]) & (solidMask.segments[y][z] ^ solidMask.segments[y][z + 1]);
                planesZ[(size_t) mask.block.type][y] = (mask.segments[z][y] | mask.segments[z + 1][y]) & (solidMask.segments[z][y] ^ solidMask.segments[z + 1][y]);
            }
        }

        for(auto& [key,mask]: masks){
            BlockType type = predefinedBlocks[mask.block.type];
            if(type.untextured) continue;
            //std::cout << "Solving plane: " << i << std::endl;
            //for(int j = 0;j < 64;j++) std::cout << std::bitset<64>(planes[i][j]) << std::endl;
            
            std::vector<Face> facesX = greedyMeshPlane64(planesX[(size_t) mask.block.type]);
            std::vector<Face> facesY = greedyMeshPlane64(planesY[(size_t) mask.block.type]);
            std::vector<Face> facesZ = greedyMeshPlane64(planesZ[(size_t) mask.block.type]);

            std::array<glm::vec3, 4> normals = {
                glm::vec3(0,0,0),
                glm::vec3(0,0,0),
                glm::vec3(0,0,0),
                glm::vec3(0,0,0)
            };
            float occlusion[4] = {0,0,0,0};

            for(auto& face: facesZ){
                std::array<glm::vec3, 4> vertices = {
                    glm::vec3(face.x              ,face.y + face.height, z + 1),
                    glm::vec3(face.x + face.width ,face.y + face.height, z + 1),
                    glm::vec3(face.x + face.width ,face.y              , z + 1),
                    glm::vec3(face.x              ,face.y              , z + 1)
                };

                int texture = z % 2;
                texture = type.repeatTexture ? type.textures[0] : type.textures[2 + texture];

                solidMesh->addQuadFaceGreedy(
                    vertices.data(),
                    normals.data(),
                    occlusion,
                    static_cast<float>(texture),
                    1,
                    face.width,
                    face.height
                );
            }

            for(auto& face: facesX){
                std::array<glm::vec3, 4> vertices = {
                    glm::vec3(z + 1, face.y + face.height, face.x             ),
                    glm::vec3(z + 1, face.y + face.height, face.x + face.width),
                    glm::vec3(z + 1, face.y              , face.x + face.width),
                    glm::vec3(z + 1, face.y              , face.x             )
                };

                int texture = z % 2;
                texture = type.repeatTexture ? type.textures[0] : type.textures[4 + texture];

                solidMesh->addQuadFaceGreedy(
                    vertices.data(),
                    normals.data(),
                    occlusion,
                    static_cast<float>(texture),
                    1,
                    face.width,
                    face.height
                );
            }

            for(auto& face: facesY){
                std::array<glm::vec3, 4> vertices = {
                    glm::vec3(face.x             ,z + 1,face.y              ),
                    glm::vec3(face.x + face.width,z + 1,face.y              ),
                    glm::vec3(face.x + face.width,z + 1,face.y + face.height),
                    glm::vec3(face.x             ,z + 1,face.y + face.height)
                };

                solidMesh->addQuadFaceGreedy(
                    vertices.data(),
                    normals.data(),
                    occlusion,
                    static_cast<float>(type.textures[0]),
                    1,
                    face.width,
                    face.height
                );
            }
        }
    }

    std::array<std::array<uint64_t, 64>, (size_t) BlockTypes::BLOCK_TYPES_TOTAL> planesX = {0};
    std::array<std::array<uint64_t, 64>, (size_t) BlockTypes::BLOCK_TYPES_TOTAL> planesY = {0};
    std::array<std::array<uint64_t, 64>, (size_t) BlockTypes::BLOCK_TYPES_TOTAL> planesZ = {0};
    

    Chunk* nextX = world.getChunk(worldPosition.x - 1,worldPosition.y,worldPosition.z);
    const ChunkMask& nextXSolid = nextX->getSolidMask();
    const auto& nextXmasks = nextX->getMasks();

    /*Chunk* nextY = world.getChunk(worldPosition.x,worldPosition.y - 1,worldPosition.z);
    const ChunkMask& nextYSolid = nextY->getSolidMask();
    const auto& nextYmasks = nextY->getMasks();*/

    Chunk* nextZ = world.getChunk(worldPosition.x,worldPosition.y,worldPosition.z - 1);
    const ChunkMask& nextZSolid = nextZ->getSolidMask();
    const auto& nextZmasks = nextZ->getMasks();

    for(int y = 0;y < CHUNK_SIZE;y++){
        for(auto& [key,mask]: masks){
            if(nextXmasks.count(key) != 0){
                const auto& nextMask = nextXmasks.at(key);
                planesX[(size_t) mask.block.type][y] =  (mask.segmentsRotated[0][y] | nextMask.segmentsRotated[63][y]) & 
                                                        (solidMask.segmentsRotated[0][y] ^ nextXSolid.segmentsRotated[63][y]);
            }

            /*if(nextYmasks.count(key) != 0){
                const auto& nextMask = nextYmasks.at(key);
                planesY[(size_t) mask.block.type][y] =  (mask.segments[y][0] | nextMask.segments[y][63]) & 
                                                        (solidMask.segments[y][0] ^ nextYSolid.segments[y][63]);
            }*/

            if(nextZmasks.count(key) != 0){
                const auto& nextMask = nextZmasks.at(key);
                planesZ[(size_t) mask.block.type][y] =  (mask.segments[0][y] | nextMask.segments[63][y]) & 
                                                        (solidMask.segments[0][y] ^ nextZSolid.segments[63][y]);
            }
        }
    }

    for(auto& [key,mask]: masks){
        BlockType type = predefinedBlocks[mask.block.type];
        if(type.untextured) continue;
        //std::cout << "Solving plane: " << i << std::endl;
        //for(int j = 0;j < 64;j++) std::cout << std::bitset<64>(planes[i][j]) << std::endl;
        
        std::vector<Face> facesX = greedyMeshPlane64(planesX[(size_t) mask.block.type]);
        std::vector<Face> facesY = greedyMeshPlane64(planesY[(size_t) mask.block.type]);
        std::vector<Face> facesZ = greedyMeshPlane64(planesZ[(size_t) mask.block.type]);

        std::array<glm::vec3, 4> normals = {
            glm::vec3(0,0,0),
            glm::vec3(0,0,0),
            glm::vec3(0,0,0),
            glm::vec3(0,0,0)
        };
        float occlusion[4] = {0,0,0,0};

        for(auto& face: facesZ){
            std::array<glm::vec3, 4> vertices = {
                glm::vec3(face.x              ,face.y + face.height, 0),
                glm::vec3(face.x + face.width ,face.y + face.height, 0),
                glm::vec3(face.x + face.width ,face.y              , 0),
                glm::vec3(face.x              ,face.y              , 0)
            };

            int texture = type.repeatTexture ? type.textures[0] : type.textures[2];

            solidMesh->addQuadFaceGreedy(
                vertices.data(),
                normals.data(),
                occlusion,
                static_cast<float>(texture),
                1,
                face.width,
                face.height
            );
        }

        for(auto& face: facesX){
            std::array<glm::vec3, 4> vertices = {
                glm::vec3(0, face.y + face.height, face.x             ),
                glm::vec3(0, face.y + face.height, face.x + face.width),
                glm::vec3(0, face.y              , face.x + face.width),
                glm::vec3(0, face.y              , face.x             )
            };

            int texture = type.repeatTexture ? type.textures[0] : type.textures[4];

            solidMesh->addQuadFaceGreedy(
                vertices.data(),
                normals.data(),
                occlusion,
                static_cast<float>(texture),
                1,
                face.width,
                face.height
            );
        }

        for(auto& face: facesY){
            std::array<glm::vec3, 4> vertices = {
                glm::vec3(face.x             , 0,face.y              ),
                glm::vec3(face.x + face.width, 0,face.y              ),
                glm::vec3(face.x + face.width, 0,face.y + face.height),
                glm::vec3(face.x             , 0,face.y + face.height)
            };

            solidMesh->addQuadFaceGreedy(
                vertices.data(),
                normals.data(),
                occlusion,
                static_cast<float>(type.textures[0]),
                1,
                face.width,
                face.height
            );
        }
    }
    //std::cout << "Vertices:" << this->solidMesh.get()->getVertices().size() << std::endl;
}


static inline bool isOnOrForwardPlane(const Plane& plane, glm::vec3 center){
    // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
    const int wd = CHUNK_SIZE / 2;
    const int h = CHUNK_SIZE / 2;

    const float r = 
        std::abs(plane.normal.x) * (wd) +
        std::abs(plane.normal.y) * (h) +
        std::abs(plane.normal.z) * (wd);

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