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

#ifdef _WIN32
DWORD WINAPI generateChunkThread(LPVOID arg) {
#else
int generateChunkThread(void *arg) {
#endif
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
        chunk->meshGenerating = 1;

        #ifdef _WIN32
        thread_t thread = CreateThread(
            NULL,                // default security attributes
            0,                   // default stack size
            generateChunkThread, // thread function
            chunk,               // argument to thread function
            0,                   // default creation flags
            NULL);               // receive thread identifier

        if (thread == THREAD_ERROR) {
            fprintf(stderr, "Error creating thread\n");
            return NULL;
        }
        #else
        thrd_t thread;

        if (thrd_create(&thread, generateChunkThread, chunk) != THREAD_SUCCESS) {
            fprintf(stderr, "Error creating thread\n");
            return NULL;
        }
        #endif

        return NULL;
    }

    if(!chunk->buffersLoaded && chunk->meshGenerated){
        if(!chunk->buffersAsigned){
            chunk->solidBuffer = newBuffer();
            chunk->solidBackBuffer = newBuffer();

            chunk->transparentBuffer = newBuffer();
            chunk->transparentBackBuffer = newBuffer();
            chunk->buffersAsigned = 1;
        }
        printf("Loading meshes %2i:%2i (%i)...\n",x,z,chunk->buffersLoaded);

        //printf("Vertices:%i Indices:%i\n", chunk->solidMesh->vertices_count, chunk->solidMesh->indices_count);
        loadMeshToBuffer(chunk->solidMesh,&chunk->solidBackBuffer);
        loadMeshToBuffer(chunk->transparentMesh,&chunk->transparentBackBuffer);

        destoryMesh(chunk->solidMesh);
        destoryMesh(chunk->transparentMesh);
        
        GLBuffer tempSolid = chunk->solidBuffer;
        GLBuffer tempTransparent = chunk->transparentBuffer;

        chunk->solidBuffer = chunk->solidBackBuffer;
        chunk->transparentBuffer =  chunk->transparentBackBuffer;

        chunk->solidBackBuffer = tempSolid;
        chunk->transparentBackBuffer = tempTransparent;

        chunk->buffersLoaded = 1;
        chunk->isDrawn = 1;
    }

   return chunk;
}

CollisionCheckResult worldCollides(World* world, float x, float y, float z){
    CollisionCheckResult result = {0};
    int range = 4;

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

                    //printf("x:%i y:%i z:%i ax:%f ay:%f az:%f\n",cx,cy,cz,x,y,z);

                    if(
                        x >= cx && x <= cx + blockWidth &&
                        y >= cy && y <= cy + blockWidth &&
                        z >= cz && z <= cz + blockWidth 
                    ){
                        result.collidedBlock = blocki;
                        result.collision = 1;
                        result.x = cx;
                        result.y = cy;
                        result.z = cz;
                        return result;
                    }
                }
                else{
                    result.collidedBlock = blocki;
                    result.collision = 1;
                    result.x = cx;
                    result.y = cy;
                    result.z = cz;
                    return result;
                }
            }
        }
    }

    return result;
}

RaycastResult raycast(World* world, float fromX, float fromY, float fromZ, float dirX, float dirY, float dirZ, float maxDistance){
    RaycastResult result = {0};
    
    float step = 0.5;
    float distance = 0;

    float x = fromX;
    float y = fromY;
    float z = fromZ;

    CollisionCheckResult check;

    while(distance < maxDistance){
        x += dirX * step;
        y += dirY * step;
        z += dirZ * step;

        check = worldCollides(world, x,y,z);
        if( check.collision){
            result.hit = 1;
            result.hitBlock = check.collidedBlock;
            result.x = check.x;
            result.y = check.y;
            result.z = check.z;
            return result;
        }

        distance += step;
    }

    return result;
}

const float DEG_TO_RAD = M_PI / 180.0f;
RaycastResult raycastFromAngles(World* world, float fromX, float fromY, float fromZ, int angleX, int angleY, float maxDistance){
    float camAngleYRad = clampAngle(clampAngle(360 - angleY) + 180) * DEG_TO_RAD;
    float camAngleXRad = clampAngle(clampAngle(360 - angleX) + 180) * DEG_TO_RAD;

    // Compute direction vector components
    float x = cos(camAngleXRad) * sin(camAngleYRad);
    float y = sin(camAngleXRad);
    float z = cos(camAngleXRad) * cos(camAngleYRad);

    float magnitude = sqrt(x * x + y * y + z * z);

    if (magnitude > 0.0f) {
        x /= magnitude;
        y /= magnitude;
        z /= magnitude;
    }

    return raycast(world,fromX,fromY,fromZ,x,y,z,10);
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