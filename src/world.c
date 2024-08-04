#include <world.h>

char* keyFromPosition(int x, int y, int z){
    FORMATED_STRING(out, "%ix%ix%i", x, y, z);
    return out;
}

World* newWorld(){
    World* world = calloc(1,sizeof(World));

    world->chunks = newHashMap();

    return world;  
}

int generateChunkThread(void* arg){
    Chunk* chunk = arg;

    chunk->solidMesh = newMesh3D();
    chunk->transparentMesh = newMesh3D();


    clock_t start = clock();

    generateMeshForChunk(chunk->solidMesh,chunk->transparentMesh,chunk);

    clock_t end = clock();
    double seconds = (double)(end - start) / (double)CLOCKS_PER_SEC;
    printf("Time generate chunk mesh: %f\n", seconds);

    chunk->meshGenerating = 0;
    chunk->meshGenerated = 1;

    return 0;
}

Chunk* generateWorldChunk(World* world, int x, int z){
    char* key = keyFromPosition(x,0,z);
    Chunk* chunk = getFromHashMap(world->chunks, key);

    if(chunk == NULL){
        //printf("Generating chunk %i:%i\n", x, z);
        //chunk = generatePlainChunk(1,2);
        chunk = generatePerlinChunk(world,x,z);
        chunk->worldX = x;
        chunk->worldZ = z;

        putIntoHashMap(chunk->world->chunks, key, chunk);
    }
    
    free(key);

    return chunk;
}

Chunk* getWorldChunk(World* world, int x, int z){
    char* key = keyFromPosition(x,0,z);
    Chunk* chunk = getFromHashMap(world->chunks, key);
    free(key);
    return chunk;
}

Chunk* getWorldChunkWithMesh(World* world, int x, int z){
    Chunk* chunk = getWorldChunk(world, x, z);
    if(chunk == NULL) return NULL;


    if(!chunk->meshGenerated && !chunk->meshGenerating){
        thrd_t thread;

        if (thrd_create(&thread, generateChunkThread, chunk) != thrd_success) {
            fprintf(stderr, "Error creating thread\n");
            return NULL;
        }

        chunk->meshGenerating = 1;
        return NULL;
    }

    if(!chunk->buffersLoaded && chunk->meshGenerated){
        chunk->buffersLoaded = 1;

        //printf("Loading meshes %2i:%2i (%i)...\n",x,z,chunk->buffersLoaded);

        //printf("Vertices:%i Indices:%i\n", chunk->solidMesh->vertices_count, chunk->solidMesh->indices_count);
        chunk->solidBuffer = newBuffer();
        loadMeshToBuffer(chunk->solidMesh,&chunk->solidBuffer);

        chunk->transparentBuffer = newBuffer();
        loadMeshToBuffer(chunk->transparentMesh,&chunk->transparentBuffer);

        destoryMesh(chunk->solidMesh);
        destoryMesh(chunk->transparentMesh);
    }

   return chunk;
}

int worldCollides(World* world, float x, float y, float z){
    int range = 1;

    int collisionFound = 0;
    float blockWidth = 1;

    for(int i = -range;i <= range;i++){
        for(int j = -range;j <= range;j++){
            for(int g = -range;g <= range;g++){
                int cx = x + i;
                int cy = y + j;
                int cz = z + g;

                BlockIndex blocki = getWorldBlock(world, cx, cy, cz);
                if(blocki >= 0){
                    BlockType block = predefinedBlocks[blocki];
                    if(!block.solid) continue;
                }
                
                if(
                    x > cx && x < cx + blockWidth &&
                    y > cy && y < cy + blockWidth &&
                    z > cz && z < cz + blockWidth 
                ){
                    collisionFound = 1;
                    printf("collision!");
                    break;
                }
            }
            if(collisionFound) break;
        }
        if(collisionFound) break;
    }

    return collisionFound;
}

BlockIndex getWorldBlock(World* world,int x, int y, int z){
    if(y < 0 || y > DEFAULT_CHUNK_HEIGHT) return INVALID_COORDINATES;
    
    int chunkX = floor((double)x / (double)DEFAULT_CHUNK_SIZE);
    int chunkZ = floor((double)z / (double)DEFAULT_CHUNK_SIZE);

    int ix = abs(x - chunkX * DEFAULT_CHUNK_SIZE);
    int iz = abs(z - chunkZ * DEFAULT_CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %i(%i)x%ix%i(%i)\n", chunkX, chunkZ, ix,y,iz);

    Chunk* chunk = getWorldChunk(world, chunkX,chunkZ);
    if(chunk == NULL) chunk = generateWorldChunk(world, chunkX, chunkZ);

    return getChunkBlock(chunk, ix, y, iz);
}

int setWorldBlock(World* world, int x, int y, int z, BlockIndex index){
    if(y < 0 || y > DEFAULT_CHUNK_HEIGHT) return INVALID_COORDINATES;
    
    int chunkX = floor((double)x / (double)DEFAULT_CHUNK_SIZE);
    int chunkZ = floor((double)z / (double)DEFAULT_CHUNK_SIZE);

    int ix = abs(x - chunkX * DEFAULT_CHUNK_SIZE);
    int iz = abs(z - chunkZ * DEFAULT_CHUNK_SIZE);
    printf("Chunk coords: %ix%i Block coords: %ix%ix%i\n", chunkX, chunkZ, ix,y,iz);

    //BlockIndex i = getWorldBlock(world, ix, y, iz);

    Chunk* chunk = getWorldChunk(world, chunkX,chunkZ);
    if(chunk == NULL) chunk = generateWorldChunk(world, chunkX, chunkZ);

    setChunkBlock(chunk, ix, y, iz,index);

    return OK;
}

void freeWorld(World* world){
    freeHashMap(world->chunks);
    free(world);
}