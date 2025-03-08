#include <game/terrain_manager.hpp>

bool TerrainManager::HandlePriorityMeshes(){
    if(has_priority_meshes && priority_count > 0){
        mesh_generator.syncGenerateAsyncUploadMesh(priority_mesh_queue[(priority_count--) - 1], BitField3D::NONE);
        return true;
    }
    else if(has_priority_meshes && priority_count == 0) has_priority_meshes = false;
    return false;
}
bool TerrainManager::loadRegion(glm::ivec3 around, int render_distance){
    if(!game_state || !game_state->world_stream) return false;
    if(loading_region) stop_loading_region = true;
    while(loading_region){};
    stop_loading_region = false;


    loading_region = true;
    std::thread th([this, around, render_distance](){
        auto& terrain = game_state->getTerrain();
        auto& world_stream = *game_state->world_stream;

        SpiralIndexer3D generation_indexer = {};
        SpiralIndexer3D meshing_indexer = {};

        for(int i = 0;i < render_distance;i++){
            std::array<std::queue<Chunk*>, thread_count> generation_queues;
            int queue_index = 0;

            while(generation_indexer.getCurrentDistance() < i + 2)
            {
                if(stop_loading_region){
                    loading_region = false;
                    return;
                }
                
                if(HandlePriorityMeshes()) continue;
                
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
                
                //world_generator.generateTerrainChunk(chunk, chunkPosition);
                generation_queues[queue_index].push(chunk);
                queue_index = (queue_index + 1) % thread_count;
            }
            std::array<std::thread, thread_count> threads;
            
            int j = 0;
            for(auto& queue: generation_queues){
                threads[j] = world_generator.threadedQueueGeneration(queue, generation_left[j], stop_loading_region);    
               j++;
            }

            for(auto& thread: threads) thread.join();
            if(stop_loading_region){
                loading_region = false;
                return;
            }
            
            while(meshing_indexer.getCurrentDistance() < i){
                if(stop_loading_region){
                    loading_region = false;
                    return;
                }

                if(HandlePriorityMeshes()) continue;

                glm::ivec3 chunkPosition = meshing_indexer.get() + around;
                meshing_indexer.next();

                auto* chunk = terrain.getChunk(chunkPosition);
                if(!chunk || chunk->isEmpty()) continue;

                auto level = calculateSimplificationLevel(around,chunkPosition);

                mesh_generator.syncGenerateAsyncUploadMesh(chunk, level);
            }
        }

        loading_region = false;
    });

    th.detach();
    return true;
}

BitField3D::SimplificationLevel TerrainManager::calculateSimplificationLevel(const glm::vec3& around, const glm::vec3& chunkPosition){
    int distance = glm::clamp(glm::distance(glm::vec3(around), glm::vec3(chunkPosition)) / 3.0f, 0.0f, 7.0f);
    return static_cast<BitField3D::SimplificationLevel>(distance - 1);
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

    if(loading_region) has_priority_meshes = true;
}
#undef regenMesh

void TerrainManager::regenerateChunkMesh(Chunk* chunk){
    if(!loading_region) mesh_generator.syncGenerateSyncUploadMesh(chunk, mesh_registry, BitField3D::NONE);
    else if(!has_priority_meshes){
        priority_mesh_queue[priority_count++] = chunk;
    }
}