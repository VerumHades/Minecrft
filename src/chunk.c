#include <chunk.h>

BlockType predefinedBlocks[] = {
    { // air
        .transparent = 1,
        .untextured = 1,
    },
    { // grass
        .textureTop = 0,
        .textureBottom = 2,
        .textureFront = 1,
        .textureBack = 1,
        .textureLeft = 1,
        .textureRight = 1,
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    },
    { // dirt
        .repeatTexture = 1,
        .textureTop = 2,
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    },
    { // stone
        .repeatTexture = 1,
        .textureTop = 3,
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    },
    { // leaf block
        .repeatTexture = 1,
        .textureTop = 6,
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    },
    { // oak log
        .textureTop = 4,
        .textureBottom = 4,
        .textureFront = 5,
        .textureBack = 5,
        .textureLeft = 5,
        .textureRight = 5,
        .colliders = (RectangularCollider[]){
            {.x = 0, .y = 0, .z = 0, .width = 1.0,.height = 1.0, .depth = 1.0}
        },
        .colliderCount = 1
    }
};

BlockIndex getChunkBlock(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z){
    if(x >= chunk->size_x) return INVALID_COORDINATES;
    if(y >= chunk->size_y) return INVALID_COORDINATES;
    if(z >= chunk->size_z) return INVALID_COORDINATES;

    ChunkLayer* layer = &chunk->layers[y];
    
    if(layer->mode == LAYER_MODE_FILL) return layer->block_index;
    else if(layer->mode == LAYER_MODE_INDIVIDUAL) return layer->data[x + z * chunk->size_z];

    return LAYER_CORRUPTED;
}

int setChunkBlock(Chunk* chunk, unsigned int x, unsigned int y, unsigned int z, BlockIndex value){
    if(x >= chunk->size_x) return INVALID_COORDINATES;
    if(y >= chunk->size_y) return INVALID_COORDINATES;
    if(z >= chunk->size_z) return INVALID_COORDINATES;

    ChunkLayer* layer = &chunk->layers[y];

    if(layer->mode == LAYER_MODE_FILL){
        layer->mode = LAYER_MODE_INDIVIDUAL;
        layer->data = calloc(DEFAULT_CHUNK_AREA, sizeof(BlockType));

        for(int i = 0;i < DEFAULT_CHUNK_AREA;i++) layer->data[i] = layer->block_index;
    }

    layer->data[x + z * chunk->size_z] = value;

    int compress = 1;
    for(int i = 0; i < chunk->size_x * chunk->size_z;i++){
        if(layer->data[i] == value) continue;
        compress = 0;
        break;
    }

    if(compress){
        layer->mode = LAYER_MODE_FILL;
        layer->block_index = value;
        free(layer->data);
    }

    return OK;
}

Chunk* generatePlainChunk(BlockIndex top, BlockIndex rest){
    Chunk* chunk = calloc(1, sizeof(Chunk));

    chunk->size_x = DEFAULT_CHUNK_SIZE;
    chunk->size_z = DEFAULT_CHUNK_SIZE;
    chunk->size_y = DEFAULT_CHUNK_HEIGHT;

    chunk->layers = calloc(DEFAULT_CHUNK_HEIGHT, sizeof(ChunkLayer));

    for(int i = 0;i < DEFAULT_CHUNK_HEIGHT;i++){
        ChunkLayer* layer = &chunk->layers[i];

        layer->mode = LAYER_MODE_FILL;

        if(i < 60) layer->block_index = 1;
        else if(i == 60) layer->block_index = 2;
        else layer->block_index = 3;
    }

    return chunk;
}

Chunk* generateEmptyChunk(World* world){
    Chunk* chunk = calloc(1, sizeof(Chunk));
    chunk->world = world;

    chunk->size_x = DEFAULT_CHUNK_SIZE;
    chunk->size_z = DEFAULT_CHUNK_SIZE;
    chunk->size_y = DEFAULT_CHUNK_HEIGHT;

    chunk->layers = calloc(DEFAULT_CHUNK_HEIGHT, sizeof(ChunkLayer));

    for(int i = 0;i < DEFAULT_CHUNK_HEIGHT;i++){
        ChunkLayer* layer = &chunk->layers[i];

        layer->mode = LAYER_MODE_FILL;
        layer->block_index = 0;
    }

    return chunk;
}

void generateTree(Chunk* chunk, int x, int y, int z){
    int trunkHeight = 4;
    BlockIndex trunkBlock = 5;
    BlockIndex leafBlock = 4;

    for(int i = 0; i < trunkHeight;i++) setChunkBlock(chunk,x,y+i,z, trunkBlock);

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
}

Chunk* generatePerlinChunk(World* world, int chunkX, int chunkZ){
    Chunk* chunk = generateEmptyChunk(world);

    for(int x = 0;x < DEFAULT_CHUNK_SIZE;x++){
        for(int z = 0;z < DEFAULT_CHUNK_SIZE;z++){
            float value = perlin((float)(x + chunkX * DEFAULT_CHUNK_SIZE) / 100.0,(float)(z + chunkZ * DEFAULT_CHUNK_SIZE) / 100.0);
            //printf("%f %i %i\n", value,x,z);
            int converted_value = floor(100 * value); 

            for(int y = 0;y < converted_value;y++){
                BlockIndex block = 0;

                if(y + 1 == converted_value){
                    block = 1;
                    if(rand() % 100 == 0) generateTree(chunk,x,converted_value,z);
                }
                else if(y + 3 >= converted_value) block = 2;
                else block = 3;

                setChunkBlock(chunk,x,y,z, block);
            }
        }
    }
    
    return chunk;
}

Vertex createVertex(float x, float y, float z){
    Vertex vertex = {0};

    vertex.size = 8;
    vertex.values[0] = x;
    vertex.values[1] = y;
    vertex.values[2] = z;

    return vertex;
}

void setVertexNormalData(Vertex* vertex, float normalX, float normalY, float normalZ){
    vertex->values[3] = normalX;
    vertex->values[4] = normalY;
    vertex->values[5] = normalZ;
}

void setVertexTextureCoordinates(Vertex* vertex, float x, float y){
    vertex->values[6] = x;
    vertex->values[7] = y;
}

#define VERTEX4NORMAL(a,b,c,d, x,y,z) setVertexNormalData(&a, x,y,z);\
                    setVertexNormalData(&b, x,y,z);\
                    setVertexNormalData(&c, x,y,z);\
                    setVertexNormalData(&d, x,y,z);\

#define VERTEX4TEXCOORDS(a,b,c,d,textureNumber,textureSize) \
    float textureX = (textureNumber % TEXTURES_TOTAL) * textureSize; \
    float textureY = (textureNumber / TEXTURES_TOTAL) * textureSize; \
    setVertexTextureCoordinates(&a, textureX, textureY); \
    setVertexTextureCoordinates(&b, textureX + textureSize, textureY); \
    setVertexTextureCoordinates(&c, textureX + textureSize, textureY + textureSize); \
    setVertexTextureCoordinates(&d, textureX, textureY + textureSize); \



#define TEX_SELECT(texture) (currentBlock.repeatTexture ? currentBlock.textureTop : texture)
#define HAS_FACE(block) block <= 0;
void generateMeshForChunk(Mesh* solid, Mesh* transparent, Chunk* chunk){
    setVertexFormat(solid, (int[]){3,3,2}, 3);
    setVertexFormat(transparent, (int[]){3,3,2}, 3);

    float textureSize = 1.0 / TEXTURES_TOTAL;
    //printf("texturesize: %f\n",textureSize);
    //printf("%p\n",chunk);
    
    for(int y = 0;y < chunk->size_y;y++){
        ChunkLayer* layer = &chunk->layers[y];
        if(layer->mode == LAYER_MODE_FILL && layer->block_index == 0) continue;

        for(int iz = 0;iz < chunk->size_z;iz++){
            for(int ix = 0;ix < chunk->size_x;ix++){
                int x = ix + chunk->worldX * DEFAULT_CHUNK_SIZE;
                int z = iz + chunk->worldZ * DEFAULT_CHUNK_SIZE;

                //printf("%ix%ix%i\n", x, y, z);
                int index = getChunkBlock(chunk, ix, y ,iz);
                if(index <= 0) continue; // error or air
                BlockType currentBlock = predefinedBlocks[index];
                if(currentBlock.untextured) continue;

                //float r = (float) x / chunk->size_x;
                //float g = (float) y / chunk->size_y;
                //float b = (float) z / chunk->size_z;

                // Front vertices
                Vertex v1 = createVertex(ix    ,y + 1,iz    );
                Vertex v2 = createVertex(ix + 1,y + 1,iz    );
                Vertex v3 = createVertex(ix + 1,y    ,iz    );
                Vertex v4 = createVertex(ix    ,y    ,iz    );

                // Back vertices
                Vertex v5 = createVertex(ix    ,y + 1,iz + 1);
                Vertex v6 = createVertex(ix + 1,y + 1,iz + 1);
                Vertex v7 = createVertex(ix + 1,y    ,iz + 1);
                Vertex v8 = createVertex(ix    ,y    ,iz + 1);

                BlockIndex top_block = getWorldBlock(chunk->world, x, y + 1, z);
                BlockIndex bottom_block = getWorldBlock(chunk->world, x, y - 1, z);

                int top    = HAS_FACE( top_block);
                int bottom = HAS_FACE( bottom_block);

                BlockIndex left_block = getWorldBlock(chunk->world, x - 1, y, z);
                BlockIndex right_block = getWorldBlock(chunk->world, x + 1, y, z);

                int left   = HAS_FACE( left_block);
                int right  = HAS_FACE( right_block);

                BlockIndex front_block = getWorldBlock(chunk->world, x, y, z - 1);
                BlockIndex back_block = getWorldBlock(chunk->world, x, y, z + 1);

                int front  = HAS_FACE( front_block);
                int back   = HAS_FACE( back_block);

                //printf("%f %f %f\n",textureY,textureY,textureSize);

                int textureTop = currentBlock.textureTop;
                int textureBottom = TEX_SELECT(currentBlock.textureBottom);
                int textureLeft = TEX_SELECT(currentBlock.textureLeft);
                int textureRight = TEX_SELECT(currentBlock.textureRight);
                int textureFront = TEX_SELECT(currentBlock.textureFront);
                int textureBack = TEX_SELECT(currentBlock.textureBack);

                if(top){
                    VERTEX4NORMAL(v5, v6, v2, v1, 0, 1, 0);
                    VERTEX4TEXCOORDS(v5, v6, v2, v1, textureTop, textureSize);

                    if(currentBlock.transparent) addQuadFaceToMesh(transparent, v5, v6, v2, v1);
                    else addQuadFaceToMesh(solid, v5, v6, v2, v1);
                }
                if(bottom){
                    VERTEX4NORMAL(v8, v7, v3, v4, 0, -1, 0);
                    VERTEX4TEXCOORDS(v8, v7, v3, v4, textureBottom, textureSize);

                    if(currentBlock.transparent) addQuadFaceToMesh(transparent, v8, v7, v3, v4);
                    else addQuadFaceToMesh(solid, v8, v7, v3, v4);
                }

                if(left){
                    VERTEX4NORMAL(v1, v5, v8, v4, -1, 0, 0);
                    VERTEX4TEXCOORDS(v1, v5, v8, v4, textureLeft, textureSize);
                    
                    if(currentBlock.transparent) addQuadFaceToMesh(transparent, v1, v5, v8, v4);
                    else addQuadFaceToMesh(solid, v1, v5, v8, v4);
                } 
                if(right){
                    VERTEX4NORMAL(v2, v6, v7, v3, 1, 0, 0);
                    VERTEX4TEXCOORDS(v2, v6, v7, v3, textureRight, textureSize);

                    if(currentBlock.transparent) addQuadFaceToMesh(transparent, v2, v6, v7, v3);
                    else addQuadFaceToMesh(solid, v2, v6, v7, v3);
                }

                if(front){
                    VERTEX4NORMAL(v1, v2, v3, v4, 0, 0, -1);
                    VERTEX4TEXCOORDS(v1, v2, v3, v4, textureFront, textureSize);

                    if(currentBlock.transparent) addQuadFaceToMesh(transparent, v1, v2, v3, v4);
                    else addQuadFaceToMesh(solid, v1, v2, v3, v4);
                }
                if(back){
                    VERTEX4NORMAL(v5, v6, v7, v8, 0, 0, 1);
                    VERTEX4TEXCOORDS(v5, v6, v7, v8, textureBack, textureSize);
                    
                    if(currentBlock.transparent) addQuadFaceToMesh(transparent, v5, v6, v7, v8);
                    else addQuadFaceToMesh(solid, v5, v6, v7, v8);
                }
            }
        }    
    }
}
