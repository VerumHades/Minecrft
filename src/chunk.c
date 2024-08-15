#include <chunk.h>
#define FNL_IMPL
#include <FastNoiseLite.h>

BlockType predefinedBlocks[] = {
    { // air
        .transparent = 1,
        .untextured = 1
    },
    { // grass
        .textures = (unsigned char[]){0,2,1,1,1,1},
        //.textures = (unsigned char[]){0},
        //.repeatTexture = 1,
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    },
    { // dirt
        .repeatTexture = 1,
        .textures = (unsigned char[]){2},
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    },
    { // stone
        .repeatTexture = 1,
        .textures = (unsigned char[]){3},
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    },
    { // leaf block
        .repeatTexture = 1,
        .textures = (unsigned char[]){6},
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    },
    { // oak log
        .textures = (unsigned char[]){4,4,5,5,5,5},
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    },
    { // birch leaf block
        .repeatTexture = 1,
        .textures = (unsigned char[]){7},
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    },
    { // birch log
        .textures = (unsigned char[]){9,9,8,8,8,8},
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    }
};

static inline void fillChunkLevel(Chunk* chunk, unsigned int y, Block value){
    for(int x = 0; x < DEFAULT_CHUNK_SIZE;x++) for(int z = 0;z < DEFAULT_CHUNK_SIZE;z++){
        setChunkBlock(chunk, x,y,z, value);
    }
}

Block* getChunkBlock(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z){
    if(x >= DEFAULT_CHUNK_SIZE) return NULL;
    if(y >= DEFAULT_CHUNK_HEIGHT) return NULL;
    if(z >= DEFAULT_CHUNK_SIZE) return NULL;
    
    return &chunk->blocks[x][y][z];
}

int setChunkBlock(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z, Block value){
    if(x >= DEFAULT_CHUNK_SIZE) return INVALID_COORDINATES;
    if(y >= DEFAULT_CHUNK_HEIGHT) return INVALID_COORDINATES;
    if(z >= DEFAULT_CHUNK_SIZE) return INVALID_COORDINATES;

    chunk->blocks[x][y][z] = value;

    return OK;
}

Chunk* generateEmptyChunk(World* world){
    Chunk* chunk = calloc(1, sizeof(Chunk));
    chunk->world = world;

    //memset(chunk->blocks, 0, sizeof(Block) * DEFAULT_CHUNK_AREA * DEFAULT_CHUNK_HEIGHT);

    return chunk;
}

void destroyChunk(Chunk* chunk){
    if(chunk->buffersAsigned){
        destroyBuffer(chunk->solidBuffer);
        destroyBuffer(chunk->solidBackBuffer);
        destroyBuffer(chunk->transparentBuffer);
        destroyBuffer(chunk->transparentBackBuffer);
    }

    if(chunk->meshGenerated){
        destroyMesh(chunk->solidMesh);
        destroyMesh(chunk->transparentMesh);
    }

    Vec3 key = (Vec3){.x = chunk->worldX, .z = chunk->worldZ};
    removeFromPositionMap(chunk->world->chunks, &key);

    free(chunk);
}


Chunk* generatePlainChunk(World* world, Block top, Block rest){
    Chunk* chunk = generateEmptyChunk(world);

    for(int i = 0;i < DEFAULT_CHUNK_HEIGHT;i++){
        
        if(i < 60) fillChunkLevel(chunk, i, rest);
        else if(i == 60) fillChunkLevel(chunk, i, top);
    }

    return chunk;
}

void generateTree(Chunk* chunk, int x, int y, int z){
    int trunkHeight = rand() % 5 + 5;
    Block trunkBlock;
    trunkBlock.typeIndex = 7;
    Block leafBlock;
    leafBlock.typeIndex = 6;

    for(int g = 0;g < 2;g++){
        for(int i = -2; i <= 2;i++){
            for(int j = -2;j <= 2;j++){
                if(j == 0 && i == 0 && g == 0) continue;
                setChunkBlock(chunk,x+i,y+trunkHeight-1+g,z+j,leafBlock);       
            }
        }
    }

    for(int i = -1; i <= 1;i++){
        for(int j = -1;j <= 1;j++){
            setChunkBlock(chunk,x+i,y+trunkHeight+1,z+j,leafBlock);       
        }
    }

    for(int i = -1; i <= 1;i++){
        for(int j = -1;j <= 1;j++){
            if(i != 0 && j != 0) continue;
            setChunkBlock(chunk,x+i,y+trunkHeight+2,z+j,leafBlock);       
        }
    }

    for(int i = 0; i < trunkHeight;i++) setChunkBlock(chunk,x,y+i,z, trunkBlock);
}

Chunk* generatePerlinChunk(World* world, int chunkX, int chunkZ){
    Chunk* chunk = generateEmptyChunk(world);

    // Create and configure noise state
    fnl_state noise = fnlCreateState();
    noise.noise_type = FNL_NOISE_PERLIN;
    noise.octaves = 3;
    //noise.frequency = 100;

    for(int x = 0;x < DEFAULT_CHUNK_SIZE;x++){
        for(int z = 0;z < DEFAULT_CHUNK_SIZE;z++){
            float rx = (float)(x + chunkX * DEFAULT_CHUNK_SIZE);
            float rz = (float)(z + chunkZ * DEFAULT_CHUNK_SIZE);

            float main = (fnlGetNoise2D(&noise, rx, rz) + 1) / 2;
            main = pow(main,4);
            //value *= perlin(rx / 20.0, rz / 20.0);
            int converted_value = floor(256 * main); 

            for(int y = 0;y < converted_value;y++){
                Block block;
                block.typeIndex = 0;

                if(y > 120){
                    block.typeIndex = 3;
                }
                else if(y + 1 == converted_value){
                    block.typeIndex = 1;
                    if(rand() % 100 == 0) generateTree(chunk,x,converted_value,z);
                }
                else if(y + 3 >= converted_value) block.typeIndex = 2;
                else block.typeIndex = 3;

                setChunkBlock(chunk,x,y,z, block);
            }
        }
    }
    
    return chunk;
}

static inline int hasFace(Block* index){
    return index != NULL && predefinedBlocks[index->typeIndex].untextured;
}

static inline Block* getBlock(Chunk* chunk, int ix, int iz, int x, int y, int z){
    if(y < 0 || y >= DEFAULT_CHUNK_HEIGHT) return NULL;
    Block* block = getChunkBlock(chunk, ix, y, iz);
    if(block == NULL) block = getWorldBlock(chunk->world, x, y, z);
    return block;
}

static inline int hasGreedyFace(Chunk* chunk, int ix, int iz, int x, int y, int z, int offset_ix, int offset_y, int offset_iz, FaceDefinition* def, int* coordinates, Block* source){
    Block* temp = getBlock(
        chunk,
        ix + def->offsetX + coordinates[0],
        iz + def->offsetZ + coordinates[2],
        x + def->offsetX + coordinates[0],
        y + def->offsetY + coordinates[1],
        z + def->offsetZ + coordinates[2]
    );

    Block* tempSolid = getChunkBlock(
        chunk,
        offset_ix,
        offset_y,
        offset_iz                         
    );

    if(tempSolid == NULL || temp == NULL) return 0;
    if(predefinedBlocks[tempSolid->typeIndex].untextured) return 0;

    //printf("A %i\n",temp->typeIndex);
    if(!hasFace(temp)) return 0;
    if(tempSolid->typeIndex != source->typeIndex) return 0;
    return 1;
}

float textureSize = 1.0 / TEXTURES_TOTAL;

static FaceDefinition faceDefinitions[] = {
    {
        .offsetX = 0, .offsetY = 1, .offsetZ = 0,
        .vertexIndexes = (int[]){4,5,1,0},
        .textureIndex = 0
    },
    {
        .offsetX = 0, .offsetY = -1, .offsetZ = 0,
        .vertexIndexes = (int[]){7,6,2,3},
        .textureIndex = 1,
        .clockwise = 1
    },
    {
        .offsetX = -1, .offsetY = 0, .offsetZ = 0,
        .vertexIndexes = (int[]){0,4,7,3},
        .textureIndex = 2,
        .clockwise = 1
    },
    {
        .offsetX = 1, .offsetY = 0, .offsetZ = 0,
        .vertexIndexes = (int[]){1,5,6,2},
        .textureIndex = 3
    },
    {
        .offsetX = 0, .offsetY = 0, .offsetZ = -1,
        .vertexIndexes = (int[]){0,1,2,3},
        .textureIndex = 4
    },
    {
        .offsetX = 0, .offsetY = 0, .offsetZ = 1,
        .vertexIndexes = (int[]){4,5,6,7},
        .textureIndex = 5,
        .clockwise = 1
    }
};

void generateMeshForChunk(Mesh* solid, Mesh* transparent, Chunk* chunk){
    setVertexFormat(solid, (int[]){3,3,2,1,3}, 5);
    setVertexFormat(transparent, (int[]){3,3,2,1,3}, 5);

    unsigned char checked[DEFAULT_CHUNK_SIZE][DEFAULT_CHUNK_HEIGHT][DEFAULT_CHUNK_SIZE][6];

    memset(checked, 0, sizeof checked);

    for(int y = 0;y < DEFAULT_CHUNK_HEIGHT;y++){
        for(int iz = 0;iz < DEFAULT_CHUNK_SIZE;iz++){
            for(int ix = 0;ix < DEFAULT_CHUNK_SIZE;ix++){
                int x = ix + chunk->worldX * DEFAULT_CHUNK_SIZE;
                int z = iz + chunk->worldZ * DEFAULT_CHUNK_SIZE;

                //printf("%ix%ix%i\n", x, y, z);
                Block* index = getChunkBlock(chunk, ix, y ,iz);
                if(index == NULL || index->typeIndex == 0) continue; // error or air
                if(index->typeIndex > 7) {
                    printf("Chunk mesh generation stopped, corrupted data (%i %i).\n", chunk->worldX, chunk->worldZ);
                    return;
                }
                BlockType* currentBlock = &predefinedBlocks[index->typeIndex];
                //printf("%i %i\n",index->typeIndex, currentBlock.untextured);
                if(currentBlock->untextured) continue;

                float lightR = 0.5;
                float lightG = 0.5;
                float lightB = 0.5;

                for(int i = 0;i < 6;i++){
                    if(checked[ix][y][iz][i]) continue;

                    FaceDefinition* def = &faceDefinitions[i];
                    if(!hasFace(getBlock(
                        chunk,
                        ix + def->offsetX,
                        iz + def->offsetZ,
                        x + def->offsetX,
                        y + def->offsetY,
                        z + def->offsetZ
                    ))) continue;

                    int coordinates[] = {0,0,0};
                    
                    int offsetAxis = 0;
                    int nonOffsetAxis[] = {0,0};

                    int width = 0;
                    int height = 1;

                    if(def->offsetX != 0){
                        nonOffsetAxis[0] = 1;
                        nonOffsetAxis[1] = 2;
                    }
                    else if(def->offsetY != 0){
                        offsetAxis = 1;
                        nonOffsetAxis[0] = 0;
                        nonOffsetAxis[1] = 2;
                    }
                    else if(def->offsetZ != 0){
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

                            if(!hasGreedyFace(chunk,ix,iz,x,y,z,offset_ix,offset_y,offset_iz,def,coordinates,index)) break;
                            if(checked[offset_ix][offset_y][offset_iz][i]) break;

                            coordinates[nonOffsetAxis[1]] += 1;
                        }

                        if(coordinates[nonOffsetAxis[1]] != 0) max_height = coordinates[nonOffsetAxis[1]] < max_height ? coordinates[nonOffsetAxis[1]] : max_height;
                        coordinates[nonOffsetAxis[1]] = 0;

                        offset_ix = ix + coordinates[0];
                        offset_iz = iz + coordinates[2]; 
                        offset_y = y + coordinates[1];

                        if(!hasGreedyFace(chunk,ix,iz,x,y,z,offset_ix,offset_y,offset_iz,def,coordinates,index)) break;
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
                    Vec3 vertices[] = {
                        {.x = ix                 , .y = y + coordinates[1],.z = iz    },
                        {.x = ix + coordinates[0], .y = y + coordinates[1],.z = iz    },
                        {.x = ix + coordinates[0], .y = y                 ,.z = iz    },
                        {.x = ix                 , .y = y                 ,.z = iz    },

                        {.x = ix                 , .y = y + coordinates[1],.z = iz + coordinates[2]},
                        {.x = ix + coordinates[0], .y = y + coordinates[1],.z = iz + coordinates[2]},
                        {.x = ix + coordinates[0], .y = y                 ,.z = iz + coordinates[2]},
                        {.x = ix                 , .y = y                 ,.z = iz + coordinates[2]}
                    };

                    //printf("%i %i %i\n", coordinates[0], coordinates[1], coordinates[2]);
                    
                    
                    //if(currentBlock->repeatTexture) printf("Index: %i\n", def->textureIndex);
                    int texture = currentBlock->repeatTexture ? currentBlock->textures[0] : currentBlock->textures[def->textureIndex];

                    Vertex metadata = {0};
                    metadata.size = 6;
                    memcpy(metadata.values, (float[]){
                        1.0, 1.0, texture,
                        lightR, lightG, lightB
                    }, sizeof(float) * metadata.size);

                    /*if(metadata.values[0] != textureX || metadata.values[1] != textureY){
                        printf("This shouldnt happen!\n");
                    }*/
                    if(def->offsetX != 0){
                        int temp = width;
                        width = height;
                        height = temp;
                    }
                    addQuadFaceToMesh(
                        solid, 
                        (Vec3[]){
                            vertices[def->vertexIndexes[0]],
                            vertices[def->vertexIndexes[1]],
                            vertices[def->vertexIndexes[2]],
                            vertices[def->vertexIndexes[3]],
                        },
                        (Vec3){.x = def->offsetX, .y = def->offsetY, .z = def->offsetZ},
                        metadata,
                        def->clockwise,
                        width,height
                    );
                }
            }
        }    
    }
    //printf("Solid: %i %i Transparent: %i %i", solid->vertices_count, solid->indices_count, transparent->vertices_count, transparent->indices_count);
}
