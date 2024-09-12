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
    for(int x = 0; x < DEFAULT_CHUNK_SIZE;x++) for(int z = 0;z < DEFAULT_CHUNK_SIZE;z++){
        chunk.setBlock(x,y,z, value);
    }
}

Chunk::Chunk(World& world, const glm::vec2& pos): world(world), worldPosition(pos) {
    
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

#define regenMesh(x,z) { \
    Chunk* temp = this->world.getChunk(x, z);\
    if(temp) temp->regenerateMesh();\
}
void Chunk::regenerateMesh(glm::vec2 blockCoords){
    this->regenerateMesh();
    if(blockCoords.x == 0) regenMesh((int) this->worldPosition.x - 1, (int) this->worldPosition.y);
    if(blockCoords.x == DEFAULT_CHUNK_SIZE - 1) regenMesh((int) this->worldPosition.x + 1, (int) this->worldPosition.y);

    if(blockCoords.y == 0) regenMesh((int) this->worldPosition.x, (int) this->worldPosition.y - 1);
    if(blockCoords.y == DEFAULT_CHUNK_SIZE - 1) regenMesh((int) this->worldPosition.x, (int) this->worldPosition.y + 1);
}
#undef regenMesh

Block* Chunk::getBlock(uint32_t x, uint32_t y, uint32_t z, int LOD){
    if(x >= DEFAULT_CHUNK_SIZE) return nullptr;
    if(y >= DEFAULT_CHUNK_HEIGHT) return nullptr;
    if(z >= DEFAULT_CHUNK_SIZE) return nullptr;

    ChunkTreeNode* currentNode = rootNode.get();
    
    int range = DEFAULT_CHUNK_SIZE;

    int minRange = pow(2, LOD);
    while(range > minRange){
        int indexX = (float) x / (float) range >= 0.5;
        int indexY = (float) y / (float) range >= 0.5;
        int indexZ = (float) z / (float) range >= 0.5;

        ChunkTreeNode* newNode = currentNode->children[indexX][indexY][indexZ].get();
        if(newNode == nullptr) return &currentNode->value;
        
        currentNode = newNode;

        range /= 2;
        x -= range * indexX;
        y -= range * indexY;
        z -= range * indexZ;
    }

    return &currentNode->value;
}

bool Chunk::setBlock(uint32_t x, uint32_t y, uint32_t z, Block value){
    if(x >= DEFAULT_CHUNK_SIZE) return false;
    if(y >= DEFAULT_CHUNK_HEIGHT) return false;
    if(z >= DEFAULT_CHUNK_SIZE) return false;

    ChunkTreeNode* currentNode = rootNode.get();
    
    int range = DEFAULT_CHUNK_SIZE;
    while(range > 1){
        int indexX = (float) x / (float) range >= 0.5;
        int indexY = (float) y / (float) range >= 0.5;
        int indexZ = (float) z / (float) range >= 0.5;

        //if(currentNode->value.type == value.type) return true;

        ChunkTreeNode* newNode = currentNode->children[indexX][indexY][indexZ].get();
        if(newNode == nullptr){
            currentNode->children[indexX][indexY][indexZ] = std::make_unique<ChunkTreeNode>();
            newNode = currentNode->children[indexX][indexY][indexZ].get();
            newNode->size = pow(range / 2, 3);
            newNode->parent = currentNode;
        }

        currentNode = newNode;

        range /= 2;
        x -= range * indexX;
        y -= range * indexY;
        z -= range * indexZ;

        //std::cout << range << std::endl;
    }

    BlockTypes oldType = currentNode->value.type;
    currentNode->value = value;

    currentNode = currentNode->parent;
    
    while(currentNode != nullptr){
        currentNode->blockCounts[(size_t) oldType]--;
        currentNode->blockCounts[(size_t) value.type]++;

        if(currentNode->blockCounts[(size_t) value.type] == currentNode->size){
            currentNode->value = value;

            for (int x = 0; x < 2; ++x) {
                for (int y = 0; y < 2; ++y) {
                    for (int z = 0; z < 2; ++z) {
                        currentNode->children[x][y][z].reset();
                    }
                }
            }
            /*std::cout << "Compressed: ";
            for(int i = 0;i < (int) BlockTypes::BLOCK_TYPES_TOTAL;i++) std::cout << currentNode->blockCounts[(size_t) value.type] << ", ";
            std::cout << std::endl;*/
        }

        currentNode = currentNode->parent;
    }

    return true;
}

static inline Block* getWorldBlockFast(Chunk* chunk, int ix, int iz, int x, int y, int z, int LOD){
    if(y < 0 || y >= DEFAULT_CHUNK_HEIGHT) return nullptr;

    Block* block = chunk->getBlock(ix, y, iz, LOD);
    if(!block) return chunk->getWorld().getBlock(x, y, z, LOD);
    return block;
}

static inline int hasGreedyFace(Chunk* chunk, int ix, int iz, int x, int y, int z, int offset_ix, int offset_y, int offset_iz, const FaceDefinition& def, int* coordinates, Block* source, int LOD){
    Block* temp = getWorldBlockFast(
        chunk,
        ix + def.offsetX + coordinates[0],
        iz + def.offsetZ + coordinates[2],
        x + def.offsetX + coordinates[0],
        y + def.offsetY + coordinates[1],
        z + def.offsetZ + coordinates[2],
        LOD
    );

    Block* tempSolid = chunk->getBlock(
        offset_ix,
        offset_y,
        offset_iz,
        LOD                         
    );

    if(!tempSolid) return 0;

    if(getBlockType(tempSolid).untextured) return 0;
    if(!getBlockType(temp).untextured) return 0;

    if(tempSolid->type != source->type) return 0;

    return 1;   
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


void Chunk::generateMeshes(int LOD){
    this->solidMesh = std::make_unique<Mesh>();
    this->solidMesh->setVertexFormat({3,3,2,1,1});
    
    bool checked[DEFAULT_CHUNK_SIZE][DEFAULT_CHUNK_HEIGHT][DEFAULT_CHUNK_SIZE][6] = {};

    for(int y = 0;y < DEFAULT_CHUNK_HEIGHT;y++){
        for(int iz = 0;iz < DEFAULT_CHUNK_SIZE;iz++){
            for(int ix = 0;ix < DEFAULT_CHUNK_SIZE;ix++){
                int x = ix + (int) this->getWorldPosition().x * DEFAULT_CHUNK_SIZE;
                int z = iz + (int) this->getWorldPosition().y * DEFAULT_CHUNK_SIZE;

                //printf("%ix%ix%i\n", x, y, z);
                Block* currentBlockRef = this->getBlock(ix, y ,iz, LOD);
                if(!currentBlockRef) continue;

                const BlockType& currentBlock = getBlockType(currentBlockRef);
                if(currentBlock.untextured) continue;

                for(int i = 0;i < 6;i++){
                    if(checked[ix][y][iz][i]) continue;

                    const FaceDefinition& def = faceDefinitions[i];
                    Block* block = getWorldBlockFast(
                        this,
                        ix + def.offsetX,
                        iz + def.offsetZ,
                        x + def.offsetX,
                        y + def.offsetY,
                        z + def.offsetZ,
                        LOD
                    );
                    if(!block) continue;
                    if(!getBlockType(block).transparent) continue;

                    int coordinates[] = {0,0,0};
                    
                    int offsetAxis = 0;
                    int nonOffsetAxis[] = {0,0};

                    int width = 0;
                    int height = 1;

                    if(def.offsetX != 0){
                        nonOffsetAxis[0] = 1;
                        nonOffsetAxis[1] = 2;
                    }
                    else if(def.offsetY != 0){
                        offsetAxis = 1;
                        nonOffsetAxis[0] = 0;
                        nonOffsetAxis[1] = 2;
                    }
                    else if(def.offsetZ != 0){
                        offsetAxis = 2;
                        nonOffsetAxis[0] = 0;
                        nonOffsetAxis[1] = 1;
                    }

                    int max_height = DEFAULT_CHUNK_SIZE+1;
                    while(1){
                        int offset_ix = ix + coordinates[0];
                        int offset_iz = iz + coordinates[2]; 
                        int offset_y = y + coordinates[1];

                        coordinates[nonOffsetAxis[1]] = 0;
                        while(1){
                            offset_ix = ix + coordinates[0];
                            offset_iz = iz + coordinates[2]; 
                            offset_y = y + coordinates[1];

                            if(!hasGreedyFace(this,ix,iz,x,y,z,offset_ix,offset_y,offset_iz,def,coordinates,currentBlockRef, LOD)) break;
                            if(checked[offset_ix][offset_y][offset_iz][i]) break;

                            coordinates[nonOffsetAxis[1]] += 1;
                        }
                        
                        if(coordinates[nonOffsetAxis[1]] != 0) max_height = coordinates[nonOffsetAxis[1]] < max_height ? coordinates[nonOffsetAxis[1]] : max_height;
                        coordinates[nonOffsetAxis[1]] = 0;

                        offset_ix = ix + coordinates[0];
                        offset_iz = iz + coordinates[2]; 
                        offset_y = y + coordinates[1];

                        if(!hasGreedyFace(this,ix,iz,x,y,z,offset_ix,offset_y,offset_iz,def,coordinates,currentBlockRef, LOD)) break;
                        if(checked[offset_ix][offset_y][offset_iz][i]) break;

                        coordinates[nonOffsetAxis[0]] += 1;
                        width++;
                        checked[offset_ix][offset_y][offset_iz][i] = true;
                    }  
                    if(max_height == DEFAULT_CHUNK_SIZE+1) continue;

                    //if(max_height == 0) printf("%i\n", max_height);
                    for(int padderH = 0;padderH < max_height;padderH++){
                        coordinates[nonOffsetAxis[1]] = padderH;
                        for(int padderW = 0;padderW < width;padderW++){
                            coordinates[nonOffsetAxis[0]] = padderW;
                            int offset_ix = ix + coordinates[0];
                            int offset_iz = iz + coordinates[2]; 
                            int offset_y = y + coordinates[1];

                            checked[offset_ix][offset_y][offset_iz][i] = true;
                        }
                    } 

                    coordinates[nonOffsetAxis[0]] = width;
                    coordinates[nonOffsetAxis[1]] = max_height;

                    height = max_height;
                    coordinates[offsetAxis] = 1;
                    //printf("%i %i %i\n", coordinates[0], coordinates[1], coordinates[2]);

                    // Front vertices
                    glm::vec3 vertices[8] = {
                        {ix                 , y + coordinates[1], iz    },
                        {ix + coordinates[0], y + coordinates[1], iz    },
                        {ix + coordinates[0], y                 , iz    },
                        {ix                 , y                 , iz    },
                        {ix                 , y + coordinates[1], iz + coordinates[2]},
                        {ix + coordinates[0], y + coordinates[1], iz + coordinates[2]},
                        {ix + coordinates[0], y                 , iz + coordinates[2]},
                        {ix                 , y                 , iz + coordinates[2]}
                    };

                    //printf("%i %i %i\n", coordinates[0], coordinates[1], coordinates[2]);
                    
                    
                    //if(currentBlock->repeatTexture) printf("Index: %i\n", def->textureIndex);
                    int texture = currentBlock.repeatTexture ? currentBlock.textures[0] : currentBlock.textures[def.textureIndex];

                    if(def.offsetX != 0){
                        int temp = width;
                        width = height;
                        height = temp;
                    }

                    glm::vec3 vertexArray[4] = {
                        vertices[def.vertexIndexes[0]],
                        vertices[def.vertexIndexes[1]],
                        vertices[def.vertexIndexes[2]],
                        vertices[def.vertexIndexes[3]]
                    };

                    glm::vec3 faceNormal = glm::vec3(def.offsetX, def.offsetY, def.offsetZ);

                    /*glm::vec3 normalArray[4] = {
                        cubeNormals[def.vertexIndexes[0]],
                        cubeNormals[def.vertexIndexes[1]],
                        cubeNormals[def.vertexIndexes[2]],
                        cubeNormals[def.vertexIndexes[3]]
                    };*/
                    glm::vec3 normalArray[4] = {
                        faceNormal,
                        faceNormal,
                        faceNormal,
                        faceNormal
                    };

                    float occlusion[4] = {
                        0,0,0,0
                    };

                    this->solidMesh->addQuadFaceGreedy(
                        vertexArray,
                        normalArray,
                        occlusion,
                        static_cast<float>(texture),
                        def.clockwise,
                        width,height
                    );
                }
            }
        }    
    }

    //std::cout << "Vertices:" << this->solidMesh.get()->getVertices().size() << std::endl;
}


static inline bool isOnOrForwardPlane(const Plane& plane, glm::vec3 center){
    // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
    const int wd = DEFAULT_CHUNK_SIZE / 2;
    const int h = DEFAULT_CHUNK_HEIGHT / 2;

    const float r = 
        std::abs(plane.normal.x) * (wd) +
        std::abs(plane.normal.y) * (h) +
        std::abs(plane.normal.z) * (wd);

    return -r <= plane.getSignedDistanceToPlane(center);
}

bool Chunk::isOnFrustum(PerspectiveCamera& camera) const {
    //Get global scale thanks to our transform
    Frustum& frustum = camera.getFrustum();
    glm::vec3 position = glm::vec3(this->worldPosition.x * DEFAULT_CHUNK_SIZE, DEFAULT_CHUNK_HEIGHT / 2, this->worldPosition.y * DEFAULT_CHUNK_SIZE);

    return isOnOrForwardPlane(frustum.leftFace, position) &&
        isOnOrForwardPlane(frustum.rightFace, position) &&
        isOnOrForwardPlane(frustum.topFace, position) &&
        isOnOrForwardPlane(frustum.bottomFace, position) &&
        isOnOrForwardPlane(frustum.nearFace, position) &&
        isOnOrForwardPlane(frustum.farFace, position);
};