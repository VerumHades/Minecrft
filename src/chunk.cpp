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

static inline void fillChunkLevel(Chunk& chunk, unsigned int y, Block value){
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
    if(blockCoords.x == 0) regenMesh(this->worldPosition.x - 1, this->worldPosition.y);
    if(blockCoords.x == DEFAULT_CHUNK_SIZE - 1) regenMesh(this->worldPosition.x + 1, this->worldPosition.y);

    if(blockCoords.y == 0) regenMesh(this->worldPosition.x, this->worldPosition.y - 1);
    if(blockCoords.y == DEFAULT_CHUNK_SIZE - 1) regenMesh(this->worldPosition.x, this->worldPosition.y + 1);
}
#undef regenMesh

Block* Chunk::getBlock(unsigned int x, unsigned int y, unsigned int z){
    if(x >= DEFAULT_CHUNK_SIZE) return nullptr;
    if(y >= DEFAULT_CHUNK_HEIGHT) return nullptr;
    if(z >= DEFAULT_CHUNK_SIZE) return nullptr;
    
    return &this->blocks[x][y][z];
}

bool Chunk::setBlock(unsigned int x, unsigned int y, unsigned int z, Block value){
    if(x >= DEFAULT_CHUNK_SIZE) return false;
    if(y >= DEFAULT_CHUNK_HEIGHT) return false;
    if(z >= DEFAULT_CHUNK_SIZE) return false;

    this->blocks[x][y][z] = value;
    return true;
}

static inline Block* getWorldBlockFast(Chunk* chunk, int ix, int iz, int x, int y, int z){
    if(y < 0 || y >= DEFAULT_CHUNK_HEIGHT) return nullptr;

    Block* block = chunk->getBlock(ix, y, iz);
    if(!block) return chunk->getWorld().getBlock(x, y, z);
    return block;
}

static inline int hasGreedyFace(Chunk* chunk, int ix, int iz, int x, int y, int z, int offset_ix, int offset_y, int offset_iz, const FaceDefinition& def, int* coordinates, Block* source){
    Block* temp = getWorldBlockFast(
        chunk,
        ix + def.offsetX + coordinates[0],
        iz + def.offsetZ + coordinates[2],
        x + def.offsetX + coordinates[0],
        y + def.offsetY + coordinates[1],
        z + def.offsetZ + coordinates[2]
    );

    Block* tempSolid = chunk->getBlock(
        offset_ix,
        offset_y,
        offset_iz                         
    );

    if(!tempSolid) return 0;

    if(getBlockType(tempSolid).untextured) return 0;
    if(!getBlockType(temp).untextured) return 0;

    if(tempSolid->type != source->type) return 0;

    return 1;   
}

float textureSize = 1.0 / TEXTURES_TOTAL;

static std::vector<FaceDefinition> faceDefinitions = {
    FaceDefinition(0, 1, 0, {4, 5, 1, 0}, 0),              // Top face
    FaceDefinition(0, -1, 0, {7, 6, 2, 3}, 1, true),       // Bottom face
    FaceDefinition(-1, 0, 0, {0, 4, 7, 3}, 2, true),       // Left face
    FaceDefinition(1, 0, 0, {1, 5, 6, 2}, 3),              // Right face
    FaceDefinition(0, 0, -1, {0, 1, 2, 3}, 4),             // Front face
    FaceDefinition(0, 0, 1, {4, 5, 6, 7}, 5, true)         // Back face
};

void Chunk::generateMeshes(){
    this->solidMesh = std::make_unique<Mesh>();
    this->solidMesh->setVertexFormat({3,3,2,1,3});

    bool checked[DEFAULT_CHUNK_SIZE][DEFAULT_CHUNK_HEIGHT][DEFAULT_CHUNK_SIZE][6] = {};

    for(int y = 0;y < DEFAULT_CHUNK_HEIGHT;y++){
        for(int iz = 0;iz < DEFAULT_CHUNK_SIZE;iz++){
            for(int ix = 0;ix < DEFAULT_CHUNK_SIZE;ix++){
                int x = ix + this->getWorldPosition().x * DEFAULT_CHUNK_SIZE;
                int z = iz + this->getWorldPosition().y * DEFAULT_CHUNK_SIZE;

                //printf("%ix%ix%i\n", x, y, z);
                Block* currentBlockRef = this->getBlock(ix, y ,iz);
                if(!currentBlockRef) continue;

                const BlockType& currentBlock = getBlockType(currentBlockRef);
                if(currentBlock.untextured) continue;
                
                float lightR = 0.5;
                float lightG = 0.5;
                float lightB = 0.5;

                for(int i = 0;i < 6;i++){
                    if(checked[ix][y][iz][i]) continue;

                    const FaceDefinition& def = faceDefinitions[i];
                    Block* block = getWorldBlockFast(
                        this,
                        ix + def.offsetX,
                        iz + def.offsetZ,
                        x + def.offsetX,
                        y + def.offsetY,
                        z + def.offsetZ
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

                    int max_height = DEFAULT_CHUNK_SIZE;
                    while(1){
                        int offset_ix = ix + coordinates[0];
                        int offset_iz = iz + coordinates[2]; 
                        int offset_y = y + coordinates[1];

                        coordinates[nonOffsetAxis[1]] = 0;
                        while(1){
                            offset_ix = ix + coordinates[0];
                            offset_iz = iz + coordinates[2]; 
                            offset_y = y + coordinates[1];

                            if(!hasGreedyFace(this,ix,iz,x,y,z,offset_ix,offset_y,offset_iz,def,coordinates,currentBlockRef)) break;
                            if(checked[offset_ix][offset_y][offset_iz][i]) break;

                            coordinates[nonOffsetAxis[1]] += 1;
                        }
                        
                        if(coordinates[nonOffsetAxis[1]] != 0) max_height = coordinates[nonOffsetAxis[1]] < max_height ? coordinates[nonOffsetAxis[1]] : max_height;
                        coordinates[nonOffsetAxis[1]] = 0;

                        offset_ix = ix + coordinates[0];
                        offset_iz = iz + coordinates[2]; 
                        offset_y = y + coordinates[1];

                        if(!hasGreedyFace(this,ix,iz,x,y,z,offset_ix,offset_y,offset_iz,def,coordinates,currentBlockRef)) break;
                        if(checked[offset_ix][offset_y][offset_iz][i]) break;

                        coordinates[nonOffsetAxis[0]] += 1;
                        width++;
                        checked[offset_ix][offset_y][offset_iz][i] = true;
                    }  
                    if(max_height == 17) continue;
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
                    std::vector<glm::vec3> vertices = {
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

                    std::vector<float> metadata = {
                        1.0, 1.0, static_cast<float>(texture),
                        lightR, lightG, lightB
                    };

                    /*if(metadata.values[0] != textureX || metadata.values[1] != textureY){
                        printf("This shouldnt happen!\n");
                    }*/
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

                    this->solidMesh->addQuadFace(
                        vertexArray,
                        glm::vec3(def.offsetX, def.offsetY,  def.offsetZ),
                        metadata,
                        def.clockwise,
                        width,height
                    );
                }
            }
        }    
    }

    //std::cout << "Faces:" << this->solidMesh.value().getVertices().size() / 12 / 4 << std::endl;
}


static inline bool isOnOrForwardPlane(const Plane& plane, glm::vec3 center){
    // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
    const float r = 
        std::abs(plane.normal.x) * (DEFAULT_CHUNK_SIZE / 2) +
        std::abs(plane.normal.y) * (DEFAULT_CHUNK_HEIGHT / 2) +
        std::abs(plane.normal.z) * (DEFAULT_CHUNK_SIZE / 2);

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