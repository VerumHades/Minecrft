#include <chunk.h>

BlockType predefinedBlocks[] = {
    { // air
        .transparent = 1,
        .untextured = 1,
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
    int trunkHeight = rand() % 5 + 5;
    BlockIndex trunkBlock = 7;
    BlockIndex leafBlock = 6;

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

    for(int x = 0;x < DEFAULT_CHUNK_SIZE;x++){
        for(int z = 0;z < DEFAULT_CHUNK_SIZE;z++){
            float rx = (float)(x + chunkX * DEFAULT_CHUNK_SIZE);
            float rz = (float)(z + chunkZ * DEFAULT_CHUNK_SIZE);

            float mountains_areas = perlin(rx / 500.0, rz / 500.0);
            float mountains = perlin(rx / 200.0, rz / 200.0);
            float main = perlin(rx / 300.0, rz / 300.0);

            main *=   mountains_areas * pow(mountains*2,2);

            //value *= perlin(rx / 20.0, rz / 20.0);
            //printf("%f %i %i\n", value,x,z);
            int converted_value = floor(256 * main); 

            for(int y = 0;y < converted_value;y++){
                BlockIndex block = 0;

                if(y > 120){
                    block = 3;
                }
                else if(y + 1 == converted_value){
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

#define TEX_SELECT(texture) (currentBlock.repeatTexture ? currentBlock.textureTop : texture)

static inline int hasFace(BlockIndex index){
    return index == 0;
}

static inline BlockIndex getBlock(Chunk* chunk, int ix, int iz, int x, int y, int z){
    if(y < 0 || y >= DEFAULT_CHUNK_HEIGHT) return INVALID_COORDINATES;
    BlockIndex block = getChunkBlock(chunk, ix, y, iz);
    if(block == INVALID_COORDINATES) block = getWorldBlock(chunk->world, x, y, z);
    return block;
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

                // Front vertices
                Vec3 vertices[] = {
                    {.x = ix    , .y = y + 1,.z = iz    },
                    {.x = ix + 1, .y = y + 1,.z = iz    },
                    {.x = ix + 1, .y = y    ,.z = iz    },
                    {.x = ix    , .y = y    ,.z = iz    },

                    {.x = ix    , .y = y + 1,.z = iz + 1},
                    {.x = ix + 1, .y = y + 1,.z = iz + 1},
                    {.x = ix + 1, .y = y    ,.z = iz + 1},
                    {.x = ix    , .y = y    ,.z = iz + 1}
                };

                float lightR = 0.5;
                float lightG = 0.5;
                float lightB = 0.5;

                for(int i = 0;i < 6;i++){
                    FaceDefinition* def = &faceDefinitions[i];
                    if(!hasFace(getBlock(
                        chunk,
                        ix + def->offsetX,
                        iz + def->offsetZ,
                        x + def->offsetX,
                        y + def->offsetY,
                        z + def->offsetZ
                    ))) continue;

                    int offsetX = 0;
                    int offsetY = 0;
                    int offsetZ = 0;

                    for(int i = 0;i < 10;i++) for(int j = 0;j < 10;j++){
                        offsetX = def->offsetX == 0 ? i : 0;
                        offsetY = def->offsetY == 0 ? (def->offsetZ != 0 ? j : i) : 0;
                        offsetZ = def->offsetZ == 0 ? j : 0;
                    }
                    
                    int texture = currentBlock.repeatTexture ? currentBlock.textures[0] : currentBlock.textures[def->textureIndex];

                    //float textureX = (texture % TEXTURES_TOTAL) * textureSize; 
                    //float textureY = (texture / TEXTURES_TOTAL) * textureSize; 

                    //printf("textureX: %f textureY: %f", textureX, textureY);

                    Vertex metadata = {0};
                    metadata.size = 6;
                    memcpy(metadata.values, (float[]){
                        1.0, 1.0, texture,
                        lightR, lightG, lightB
                    }, sizeof(float) * metadata.size);

                    /*if(metadata.values[0] != textureX || metadata.values[1] != textureY){
                        printf("This shouldnt happen!\n");
                    }*/

                    addQuadFaceToMesh(
                        solid, 
                        vertices[def->vertexIndexes[0]],
                        vertices[def->vertexIndexes[1]],
                        vertices[def->vertexIndexes[2]],
                        vertices[def->vertexIndexes[3]],
                        (Vec3){.x = def->offsetX, .y = def->offsetY, .z = def->offsetZ},
                        metadata,
                        def->clockwise
                    );
                }
            }
        }    
    }
    constructMesh(solid);
    constructMesh(transparent);
    //printf("Solid: %i %i Transparent: %i %i", solid->vertices_count, solid->indices_count, transparent->vertices_count, transparent->indices_count);
}
