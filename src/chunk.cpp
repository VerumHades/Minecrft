#include <chunk.hpp>

std::map<BlockTypeEnum, BlockType> predefinedBlocks = {
    {BlockTypeEnum::Air, BlockType(true, true)}, // Air
    {BlockTypeEnum::Grass, BlockType(false, false, false, {0, 2, 1, 1, 1, 1}, {{0, 0, 0, 1.0f, 1.0f, 1.0f}})}, // Grass
    {BlockTypeEnum::Dirt, BlockType(false, false, true, {2}, {{0, 0, 0, 1.0f, 1.0f, 1.0f}})}, // Dirt
    {BlockTypeEnum::Stone, BlockType(false, false, true, {3}, {{0, 0, 0, 1.0f, 1.0f, 1.0f}})}, // Stone
    {BlockTypeEnum::LeafBlock, BlockType(false, false, true, {6}, {{0, 0, 0, 1.0f, 1.0f, 1.0f}})}, // Leaf Block
    {BlockTypeEnum::OakLog, BlockType(false, false, false, {4, 4, 5, 5, 5, 5}, {{0, 0, 0, 1.0f, 1.0f, 1.0f}})}, // Oak Log
    {BlockTypeEnum::BirchLeafBlock, BlockType(false, false, true, {7}, {{0, 0, 0, 1.0f, 1.0f, 1.0f}})}, // Birch Leaf Block
    {BlockTypeEnum::BirchLog, BlockType(false, false, false, {9, 9, 8, 8, 8, 8}, {{0, 0, 0, 1.0f, 1.0f, 1.0f}})}, // Birch Log
    {BlockTypeEnum::BlueWool, BlockType(false, false, true, {10}, {{0, 0, 0, 1.0f, 1.0f, 1.0f}})}, // Blue Wool
    {BlockTypeEnum::Sand, BlockType(false, false, true, {11}, {{0, 0, 0, 1.0f, 1.0f, 1.0f}})} // Sand
};

static inline void fillChunkLevel(Chunk& chunk, unsigned int y, Block value){
    for(int x = 0; x < DEFAULT_CHUNK_SIZE;x++) for(int z = 0;z < DEFAULT_CHUNK_SIZE;z++){
        chunk.setBlock(x,y,z, value);
    }
}

Chunk::Chunk(World& world, const glm::vec2& pos): world(world), worldPosition(pos) {

}

World& Chunk::getWorld(){
    return this->world;
}

void Chunk::regenerateMesh(){
    if(!this->buffersLoaded) return;

    this->meshGenerated = 0;
    this->meshGenerating = 0;
    this->buffersLoaded = 0;
}


const Block& Chunk::getBlock(unsigned int x, unsigned int y, unsigned int z){
    if(x >= DEFAULT_CHUNK_SIZE) throw std::invalid_argument("Invalid coordinates");
    if(y >= DEFAULT_CHUNK_HEIGHT) throw std::invalid_argument("Invalid coordinates");
    if(z >= DEFAULT_CHUNK_SIZE) throw std::invalid_argument("Invalid coordinates");
    
    return this->blocks[x][y][z];
}

void Chunk::setBlock(unsigned int x, unsigned int y, unsigned int z, Block value){
    if(x >= DEFAULT_CHUNK_SIZE) throw std::invalid_argument("Invalid coordinates");
    if(y >= DEFAULT_CHUNK_HEIGHT) throw std::invalid_argument("Invalid coordinates");
    if(z >= DEFAULT_CHUNK_SIZE) throw std::invalid_argument("Invalid coordinates");

    this->blocks[x][y][z] = value;
}

static inline const Block& getWorldBlockFast(Chunk* chunk, int ix, int iz, int x, int y, int z){
    if(y < 0 || y >= DEFAULT_CHUNK_HEIGHT) throw std::invalid_argument("Invalid coordinates");

    try{
        return chunk->getBlock(ix, y, iz);
    }
    catch(const std::exception& err){
        return chunk->getWorld().getBlock(x, y, z).value();
    }
}

static inline int hasGreedyFace(Chunk* chunk, int ix, int iz, int x, int y, int z, int offset_ix, int offset_y, int offset_iz, const FaceDefinition& def, int* coordinates, const Block& source){
    try{
        const Block& temp = getWorldBlockFast(
            chunk,
            ix + def.offsetX + coordinates[0],
            iz + def.offsetZ + coordinates[2],
            x + def.offsetX + coordinates[0],
            y + def.offsetY + coordinates[1],
            z + def.offsetZ + coordinates[2]
        );

        const Block& tempSolid = chunk->getBlock(
            offset_ix,
            offset_y,
            offset_iz                         
        );

        if(predefinedBlocks[tempSolid.type].untextured) return 0;
        if(!predefinedBlocks[temp.type].untextured) return 0;
        
        if(tempSolid.type != source.type) return 0;

        return 1;   
    }
    catch(const std::exception& err){
        return 0;
    }
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
    this->solidMesh = Mesh();
    this->transparentMesh = Mesh();

    this->solidMesh.value().setVertexFormat({3,3,2,1,3});
    this->transparentMesh.value().setVertexFormat({3,3,2,1,3});

    unsigned char checked[DEFAULT_CHUNK_SIZE][DEFAULT_CHUNK_HEIGHT][DEFAULT_CHUNK_SIZE][6];

    memset(checked, 0, sizeof checked);

    for(int y = 0;y < DEFAULT_CHUNK_HEIGHT;y++){
        for(int iz = 0;iz < DEFAULT_CHUNK_SIZE;iz++){
            for(int ix = 0;ix < DEFAULT_CHUNK_SIZE;ix++){
                int x = ix + this->getWorldPosition().x * DEFAULT_CHUNK_SIZE;
                int z = iz + this->getWorldPosition().y * DEFAULT_CHUNK_SIZE;

                //printf("%ix%ix%i\n", x, y, z);
                try{
                    const Block& index = this->getBlock(ix, y ,iz);

                    if(index.type == BlockTypeEnum::Air) continue; // error or air
                    const BlockType& currentBlock = predefinedBlocks[index.type];
                    //printf("%i %i\n",index->typeIndex, currentBlock.untextured);
                    if(currentBlock.untextured) continue;

                    float lightR = 0.5;
                    float lightG = 0.5;
                    float lightB = 0.5;

                    for(int i = 0;i < 6;i++){
                        if(checked[ix][y][iz][i]) continue;

                        const FaceDefinition& def = faceDefinitions[i];
                        if(predefinedBlocks[getWorldBlockFast(
                            this,
                            ix + def.offsetX,
                            iz + def.offsetZ,
                            x + def.offsetX,
                            y + def.offsetY,
                            z + def.offsetZ
                        ).type].untextured) continue;

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

                        int max_height = 17;
                        while(1){
                            int offset_ix = ix + coordinates[0];
                            int offset_iz = iz + coordinates[2]; 
                            int offset_y = y + coordinates[1];

                            coordinates[nonOffsetAxis[1]] = 0;
                            while(1){
                                offset_ix = ix + coordinates[0];
                                offset_iz = iz + coordinates[2]; 
                                offset_y = y + coordinates[1];

                                if(!hasGreedyFace(this,ix,iz,x,y,z,offset_ix,offset_y,offset_iz,def,coordinates,index)) break;
                                if(checked[offset_ix][offset_y][offset_iz][i]) break;

                                coordinates[nonOffsetAxis[1]] += 1;
                            }

                            if(coordinates[nonOffsetAxis[1]] != 0) max_height = coordinates[nonOffsetAxis[1]] < max_height ? coordinates[nonOffsetAxis[1]] : max_height;
                            coordinates[nonOffsetAxis[1]] = 0;

                            offset_ix = ix + coordinates[0];
                            offset_iz = iz + coordinates[2]; 
                            offset_y = y + coordinates[1];

                            if(!hasGreedyFace(this,ix,iz,x,y,z,offset_ix,offset_y,offset_iz,def,coordinates,index)) break;
                            if(checked[offset_ix][offset_y][offset_iz][i]) break;

                            coordinates[nonOffsetAxis[0]] += 1;
                            width++;
                            checked[offset_ix][offset_y][offset_iz][i] = 1;
                        }  
                        //if(max_height == 0) printf("%i\n", max_height);
                        for(int padderH = 0;padderH < max_height;padderH++){
                            coordinates[nonOffsetAxis[1]] = padderH;
                            for(int padderW = 0;padderW < width;padderW++){
                                coordinates[nonOffsetAxis[0]] = padderW;
                                int offset_ix = ix + coordinates[0];
                                int offset_iz = iz + coordinates[2]; 
                                int offset_y = y + coordinates[1];

                                checked[offset_ix][offset_y][offset_iz][i] = 1;
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
                            1.0, 1.0, (float) texture,
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

                        this->solidMesh.value().addQuadFace(
                            vertexArray,
                            glm::vec3(def.offsetX, def.offsetY,  def.offsetZ),
                            metadata,
                            def.clockwise,
                            width,height
                        );
                    }
                }
                catch(...){

                }
            }
        }    
    }
    //printf("Solid: %i %i Transparent: %i %i", solid->vertices_count, solid->indices_count, transparent->vertices_count, transparent->indices_count);
}
