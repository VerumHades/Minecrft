#include <game/terrain_manager.hpp>


void TerrainManager::generateRegion(glm::ivec3 around, int render_distance){
    if(!game_state || !game_state->world_stream) return;

    auto& terrain = game_state->getTerrain();
    auto& world_stream = *game_state->world_stream;

    int pregenDistance = render_distance + 1; 

    std::array<std::queue<Chunk*>, thread_count> generation_queues;
    int queue_index = 0;

    int total = 0;
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
        total++;
    }

    if(total >= 100 * 8) generating_world = true;

    //timer.timestamp("Setup queues");
    world_generator.prepareHeightMaps(around, render_distance);
    //timer.timestamp("Perpared height maps");

    std::array<std::thread, thread_count> threads;
    
    int i = 0;
    for(auto& queue: generation_queues){
        threads[i] = world_generator.threadedQueueGeneration(queue, &generation_left[i]);    
        i++;
    }

    for(auto& thread: threads) thread.join();

    generating_world = false;
}

bool TerrainManager::loadRegion(glm::ivec3 around, int render_distance){
    if(!game_state || !game_state->world_stream) return false;
    if(loading_region) return false;

    loading_region = true;
    std::thread th([this, around, render_distance](){
        generateRegion(around, render_distance);
        meshRegion(around,render_distance);
        loading_region = false;
    });

    th.detach();
    return true;
}

BitField3D::SimplificationLevel calculateSimplificationLevel(const glm::vec3& around, const glm::vec3& chunkPosition){
    int distance = glm::clamp(glm::distance(glm::vec3(around), glm::vec3(chunkPosition)) / 3.0f, 0.0f, 7.0f);
    return static_cast<BitField3D::SimplificationLevel>(distance - 1);
}

void TerrainManager::meshRegion(glm::ivec3 around, int render_distance){
    if(!game_state) return;

    if(generating_meshes){
        stop_mesh_generation = true;
        while(generating_meshes) {}
    }

    auto& terrain = game_state->getTerrain();
    
    std::thread thread = std::thread([this, &terrain, render_distance, around] {
        SpiralIndexer3D indexer = {};

        generating_meshes = true;

        while(indexer.getCurrentDistance() < render_distance)
        {
            if(stop_mesh_generation) break;

            glm::ivec3 chunkPosition = indexer.get() + around;
            indexer.next();

            auto* chunk = terrain.getChunk(chunkPosition);
            if(!chunk) continue;

            int distance = glm::clamp(glm::distance(glm::vec3(around), glm::vec3(chunkPosition)) / 3.0f, 0.0f, 7.0f);
            auto level = static_cast<BitField3D::SimplificationLevel>(distance - 1);

            chunk->current_simplification = level;
        }
        
        indexer = {};
        
        while(indexer.getCurrentDistance() < render_distance){
            if(stop_mesh_generation) break;

            if(has_priority_meshes && priority_count > 0){
                mesh_generator.syncGenerateAsyncUploadMesh(priority_mesh_queue[(priority_count--) - 1], BitField3D::NONE);
                continue;
            }
            else if(has_priority_meshes && priority_count == 0) has_priority_meshes = false;

            glm::ivec3 chunkPosition = indexer.get() + around;
            indexer.next();

            auto* chunk = terrain.getChunk(chunkPosition);
            if(!chunk || chunk->isEmpty()) continue;

            int distance = glm::clamp(glm::distance(glm::vec3(around), glm::vec3(chunkPosition)) / 3.0f, 0.0f, 7.0f);
            auto level = static_cast<BitField3D::SimplificationLevel>(distance - 1);

            mesh_generator.syncGenerateAsyncUploadMesh(chunk, level);
        }

        stop_mesh_generation = false;
        generating_meshes = false;
    });

    thread.detach();
}

bool TerrainManager::uploadPendingMeshes(){
    return mesh_generator.loadMeshFromQueue(mesh_registry, 25);
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

    if(generating_meshes) has_priority_meshes = true;
}
#undef regenMesh

void TerrainManager::regenerateChunkMesh(Chunk* chunk){
    if(!generating_meshes) mesh_generator.syncGenerateSyncUploadMesh(chunk, mesh_registry, BitField3D::NONE);
    else if(!has_priority_meshes){
        priority_mesh_queue[priority_count++] = chunk;
    }
}