#include <game/terrain_manager.hpp>


void TerrainManager::generateRegion(glm::ivec3 around, int render_distance){
    if(!game_state || !game_state->world_stream) return;

    auto& terrain = game_state->getTerrain();
    auto& world_stream = *game_state->world_stream;

    int pregenDistance = render_distance + 1; 

    const int thread_count = 8;
    std::array<std::queue<Chunk*>, thread_count> generation_queues;
    int queue_index = 0;

    //world->getWorldGenerator().generateChunkRegion(*world, {0,0,0});

    ScopeTimer timer("Generated chunks");
    
    /*for(int x = -pregenDistance; x <= pregenDistance; x++) 
    for(int z = -pregenDistance; z <= pregenDistance; z++)
    for(int y = -pregenDistance; y <= pregenDistance; y++) 
    {
        glm::ivec3 chunkPosition = glm::ivec3(x,y,z) + around;

        if(terrain.getChunk(chunkPosition)) continue;
        auto* chunk = terrain.createEmptyChunk(chunkPosition);

        if(world_stream.hasChunkAt(chunkPosition)){
            world_stream.load(chunk);
            return;
        }

        world_generator.generateTerrainChunk(chunk, chunkPosition);
    }*/

    for(int x = -pregenDistance; x <= pregenDistance; x++) 
    for(int z = -pregenDistance; z <= pregenDistance; z++)
    for(int y = -pregenDistance; y <= pregenDistance; y++) 
    {
        glm::ivec3 chunkPosition = glm::ivec3(x,y,z) + around;

        if(terrain.getChunk(chunkPosition)) continue;
        auto* chunk = terrain.createEmptyChunk(chunkPosition);

        if(world_stream.hasChunkAt(chunkPosition)){
            world_stream.load(chunk);
            continue;
        }

        generation_queues[queue_index].push(chunk);
        queue_index = (queue_index + 1) % thread_count;
    }

    timer.timestamp("Setup queues");
    world_generator.prepareHeightMaps(around, render_distance);
    timer.timestamp("Perpared height maps");

    std::array<std::thread, thread_count> threads;
    
    int i = 0;
    for(auto& queue: generation_queues){
        threads[i++] = world_generator.threadedQueueGeneration(queue);    
    }

    for(auto& thread: threads) thread.join();
}

void TerrainManager::loadRegion(glm::ivec3 around, int render_distance){
    if(!game_state || !game_state->world_stream) return;

    auto& terrain = game_state->getTerrain();

    generateRegion(around, render_distance);
    meshRegion(around,render_distance);
}

void TerrainManager::meshRegion(glm::ivec3 around, int render_distance){
    if(!game_state || generating_meshes) return;

    auto& terrain = game_state->getTerrain();
    
    std::thread thread = std::thread([this, &terrain, render_distance, around] {
        SpiralIndexer3D indexer = {};

        generating_meshes = true;

        while(indexer.getCurrentDistance() < render_distance){
            if(priority){
                mesh_generator.syncGenerateAsyncUploadMesh(priority, BitField3D::NONE);
                priority = nullptr;
                continue;
            }

            glm::ivec3 chunkPosition = indexer.get() + around;
            indexer.next();

            auto* chunk = terrain.getChunk(chunkPosition);
            if(!chunk) continue;

            int distance = glm::clamp(glm::distance(glm::vec3(around), glm::vec3(chunkPosition)) / 3.0f, 0.0f, 7.0f);

            auto level = static_cast<BitField3D::SimplificationLevel>(distance - 1);

            mesh_generator.syncGenerateAsyncUploadMesh(chunk, level);
        }
        generating_meshes = false;
    });

    thread.detach();
}

void TerrainManager::uploadPendingMeshes(){
    mesh_generator.loadMeshFromQueue(mesh_registry, 25);
}

#define regenMesh(position) { \
    Chunk* temp = game_state->getTerrain().getChunk(position);\
    if(temp) regenerateChunkMesh(temp);\
}
void TerrainManager::regenerateChunkMesh(Chunk* chunk, glm::vec3 blockCoords){
    regenerateChunkMesh(chunk);
    if(blockCoords.x == 0)              regenMesh(chunk->getWorldPosition() - glm::ivec3(1,0,0));
    if(blockCoords.x == CHUNK_SIZE - 1) regenMesh(chunk->getWorldPosition() + glm::ivec3(1,0,0));

    if(blockCoords.y == 0)              regenMesh(chunk->getWorldPosition() - glm::ivec3(0,1,0));
    if(blockCoords.y == CHUNK_SIZE - 1) regenMesh(chunk->getWorldPosition() + glm::ivec3(0,1,0));

    if(blockCoords.z == 0)              regenMesh(chunk->getWorldPosition() - glm::ivec3(0,0,1));
    if(blockCoords.z == CHUNK_SIZE - 1) regenMesh(chunk->getWorldPosition() + glm::ivec3(0,0,1));
}
#undef regenMesh

void TerrainManager::regenerateChunkMesh(Chunk* chunk){
    if(!generating_meshes) mesh_generator.syncGenerateSyncUploadMesh(chunk, mesh_registry, BitField3D::NONE);
    else priority = chunk;
}

/*void TerrainManager::loadChunk(const glm::ivec3& position){
    auto* chunk = game_state->getTerrain().getChunk(position);
    if(chunk) return;

    chunk = terrain.createEmptyChunk(position);

    if(world_stream && world_stream->hasChunkAt(position)){
        world_stream->load(chunk);
        return;
    }

    world_generator.generateTerrainChunk(chunk, position);
}

void TerrainManager::unloadChunk(const glm::ivec3& position){
    auto* chunk = terrain.getChunk(position);
    if(!chunk) return;

    if(world_stream) world_stream->save(*chunk);
    terrain.removeChunk(position);
}*/
