#include <game/terrain_manager.hpp>

TerrainManager::TerrainManager(){
    reset_loader();
}

bool TerrainManager::GenerateRegion(const glm::ivec3& around, int render_distance){
    if(!game_state || !game_state->world_stream) return false;

    generating_region = true;

    auto& terrain = game_state->getTerrain();
    auto& world_stream = *game_state->world_stream;
    
    SpiralIndexer3D generation_indexer = {};
    
    generated_distance = 0;

    for(int i = 1;i < render_distance + 1;i++){
        //std::array<std::queue<Chunk*>, thread_count> generation_queues;
        //int queue_index = 0;

        while(generation_indexer.getCurrentDistance() < i)
        {
            if(stop_generating_region){
                generating_region = false;
                return false;
            }

            glm::ivec3 chunkPosition = generation_indexer.get() + around;
            generation_indexer.next();

            auto* chunk = terrain.getChunk(chunkPosition);
            auto level = calculateSimplificationLevel(around,chunkPosition);

            if(chunk){
                chunk->current_simplification = level;
                continue;
            }
            chunk = terrain.createEmptyChunk(chunkPosition);
            chunk->current_simplification = level;

            if(world_stream.hasChunkAt(chunkPosition)){
                world_stream.load(chunk);
                continue;
            }
            
            world_generator.generateTerrainChunk(chunk, chunkPosition);
            //generation_queues[queue_index].push(chunk);
            //queue_index = (queue_index + 1) % thread_count;
        }

        generated_distance = i - 1;
        ///std::array<std::thread, thread_count> threads;            

        //int j = 0;
        //for(auto& queue: generation_queues){
        //    threads[j] = world_generator.threadedQueueGeneration(queue, generation_left[j], stop_loading_region);    
        //    j++;
        //}

        //for(auto& thread: threads) thread.join();
    }

    generating_region = false;

    return true;
}

bool TerrainManager::MeshRegion(const glm::ivec3& around, int render_distance){
    if(!game_state || !game_state->world_stream) return false;

    meshing_region = true;

    SpiralIndexer3D meshing_indexer = {};
    auto& terrain = game_state->getTerrain();
    auto& world_stream = *game_state->world_stream;

    while(generated_distance < render_distance){
        if(stop_meshing_region){
            meshing_region = false;
            return false;
        }

        if(HandlePriorityMeshes()) continue;

        while(meshing_indexer.getCurrentDistance() < generated_distance){
            if(stop_meshing_region){
                meshing_region = false;
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if(HandlePriorityMeshes()) continue;

            glm::ivec3 chunkPosition = meshing_indexer.get() + around;
            meshing_indexer.next();

            auto* chunk = terrain.getChunk(chunkPosition);
            if(!chunk) continue;

            auto level = calculateSimplificationLevel(around,chunkPosition);

            mesh_generator.syncGenerateAsyncUploadMesh(chunk, create_mesh(), level);
        }
    }

    meshing_region = false;

    return true;
}

bool TerrainManager::HandlePriorityMeshes(){
    if(has_priority_meshes && priority_count > 0){
        mesh_generator.syncGenerateAsyncUploadMesh(priority_mesh_queue[(priority_count--) - 1], create_mesh(), BitField3D::NONE);
        return true;
    }
    else if(has_priority_meshes && priority_count == 0) has_priority_meshes = false;
    return false;
}
bool TerrainManager::loadRegion(glm::ivec3 around, int render_distance){
    if(!game_state || !game_state->world_stream) return false;

    if(mesh_loader->DrawFailed()){
        create_mesh = [](){ return std::make_unique<PooledMesh>(); };
        reset_loader = [this](){
            mesh_loader = std::make_unique<PooledMeshLoader>();
            mesh_registry.SetMeshLoader(mesh_loader.get());
        };
    }

    StopGeneratingRegion();
    StopMeshingRegion();

    generated_distance = 0;

    std::thread th1(std::bind(&TerrainManager::GenerateRegion, this, around, render_distance));
    std::thread th2(std::bind(&TerrainManager::MeshRegion, this, around, render_distance));

    th1.detach();
    th2.detach();

    return true;
}

BitField3D::SimplificationLevel TerrainManager::calculateSimplificationLevel(const glm::vec3& around, const glm::vec3& chunkPosition){
    int distance = glm::clamp(glm::distance(glm::vec3(around), glm::vec3(chunkPosition)) / 3.0f, 0.0f, 7.0f);
    return static_cast<BitField3D::SimplificationLevel>(distance - 1);
}

bool TerrainManager::uploadPendingMeshes(){
    return mesh_generator.loadMeshFromQueue(mesh_registry, 25);
}

void TerrainManager::unloadAll(){
    mesh_registry.clear();
    reset_loader();
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

    if(meshing_region) has_priority_meshes = true;
}
#undef regenMesh

void TerrainManager::regenerateChunkMesh(Chunk* chunk){
    if(!meshing_region) mesh_generator.syncGenerateSyncUploadMesh(chunk, mesh_registry, create_mesh(), BitField3D::NONE);
    else if(!has_priority_meshes){
        priority_mesh_queue[priority_count++] = chunk;
    }
}