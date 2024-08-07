#include <world.h>

World* newWorld(){
    World* world = calloc(1,sizeof(World));

    world->chunks = newPositionMap();

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


    struct timespec start, end;

    // Start timer
    clock_gettime(CLOCK_REALTIME, &start);

    generateMeshForChunk(chunk->solidMesh,chunk->transparentMesh,chunk);

    // End timer
    clock_gettime(CLOCK_REALTIME, &end);

    // Calculate time difference
    double elapsed_time = (end.tv_sec - start.tv_sec) +
                         (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Time generate chunk mesh: %fs\n", elapsed_time);

    chunk->meshGenerating = 0;
    chunk->meshGenerated = 1;

    return 0;
}

void regenerateChunkMesh(Chunk* chunk){
    if(!chunk->buffersLoaded) return;

    chunk->meshGenerated = 0;
    chunk->meshGenerating = 0;
    chunk->buffersLoaded = 0;
}

Chunk* generateWorldChunk(World* world, int x, int z){
    Vec3 key = (Vec3){.x = x, .z = z};

    Chunk* chunk = getFromPositionMap(world->chunks, &key);

    if(chunk == NULL){
        //printf("Generating chunk %i:%i %s\n", x, z, key);
        //chunk = generatePlainChunk(1,2);
        chunk = generatePerlinChunk(world,x,z);
        chunk->worldX = x;
        chunk->worldZ = z;

        putIntoPositionMap(chunk->world->chunks, &key, chunk);
    }

    return chunk;
}

Chunk* getWorldChunk(World* world, int x, int z){
    Vec3 key = (Vec3){.x = x, .z = z};
    Chunk* chunk = getFromPositionMap(world->chunks, &key);

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
        //printf("Loading meshes %2i:%2i (%i)...\n",x,z,chunk->buffersLoaded);

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

CollisionCheckResult checkForPointCollision(World* world, float x, float y, float z, int includeRectangularColliderLess){
    CollisionCheckResult result = {0};
    int range = 3;

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
                    if(block.colliderCount == 0 && !includeRectangularColliderLess) continue;

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

CollisionCheckResult checkForRectangularCollision(World* world, float x, float y, float z, RectangularCollider* collider){
    CollisionCheckResult result = {0};
    int range = 3;

    for(int i = -range;i <= range;i++){
        for(int j = -range;j <= range;j++){
            for(int g = -range;g <= range;g++){
                int cx = (int)floor(x + i);
                int cy = (int)floor(y + j);
                int cz = (int)floor(z + g);

                BlockIndex blocki = getWorldBlock(world, cx, cy, cz);
                if(blocki >= 0){
                    BlockType block = predefinedBlocks[blocki];
                    if(block.colliderCount == 0) continue;

                    //printf("x:%i y:%i z:%i ax:%f ay:%f az:%f\n",cx,cy,cz,x,y,z);

                    for(int colliderIndex = 0;colliderIndex < block.colliderCount;colliderIndex++){
                        RectangularCollider* blockCollider = &block.colliders[colliderIndex];
                        float colliderX = blockCollider->x + cx;
                        float colliderY = blockCollider->y + cy;
                        float colliderZ = blockCollider->z + cz;
                        
                        //printf("%f %f %f %f %f %f\n", colliderX, colliderY, colliderZ, blockCollider->width, blockCollider->height, blockCollider->depth);
            
                        if(
                            x + collider->x + collider->width  >= colliderX && x + collider->x <= colliderX + blockCollider->width &&
                            y + collider->y + collider->height >= colliderY && y + collider->y <= colliderY + blockCollider->height &&
                            z + collider->z + collider->depth  >= colliderZ && z + collider->z <= colliderZ + blockCollider->depth 
                        ){
                            result.collidedBlock = blocki;
                            result.collision = 1;
                            result.x = cx;
                            result.y = cy;
                            result.z = cz;
                            return result;
                        }
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
        result.lastX = x;
        result.lastY = y;
        result.lastZ = z;

        x += dirX * step;
        y += dirY * step;
        z += dirZ * step;

        check = checkForPointCollision(world, x,y,z, 0);
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

Chunk* getChunkFromBlockPosition(World* world, int x, int z){
    int cx,cz;
    if(x >= 0) cx = x / DEFAULT_CHUNK_SIZE;
    else cx = x / DEFAULT_CHUNK_SIZE - 1;
    if(z >= 0) cz = z / DEFAULT_CHUNK_SIZE;
    else cz = z / DEFAULT_CHUNK_SIZE - 1;

    return getWorldChunk(world, cx, cz);
}

int setWorldBlock(World* world, int x, int y, int z, BlockIndex index){
    if(y < 0 || y > DEFAULT_CHUNK_HEIGHT) return INVALID_COORDINATES;
    
    int chunkX = floor((double)x / (double)DEFAULT_CHUNK_SIZE);
    int chunkZ = floor((double)z / (double)DEFAULT_CHUNK_SIZE);

    int ix = abs(x - chunkX * DEFAULT_CHUNK_SIZE);
    int iz = abs(z - chunkZ * DEFAULT_CHUNK_SIZE);
    //printf("Chunk coords: %ix%i Block coords: %ix%ix%i\n", chunkX, chunkZ, ix,y,iz);

    //BlockIndex i = getWorldBlock(world, ix, y, iz);

    Chunk* chunk = getWorldChunk(world, chunkX,chunkZ);
    if(chunk == NULL) chunk = generateWorldChunk(world, chunkX, chunkZ);

    setChunkBlock(chunk, ix, y, iz,index);

    return OK;
}

void freeWorld(World* world){
    freePositionMap(world->chunks);
    free(world);
}