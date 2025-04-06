#include <game/terrain_manager.hpp>

TerrainManager::TerrainManager(){
    service_manager = std::make_unique<Service>();

    reset_loader();

    service_manager->AddModule("mesher", [this](std::atomic<bool>& should_stop){
        if(!game_state || !game_state->world_stream) return;

        glm::ivec3 around = glm::ivec3{around_x.load(),around_y.load(),around_z.load()};

        SpiralIndexer meshing_indexer = {};
        auto& terrain = game_state->getTerrain();
        auto& world_stream = *game_state->world_stream;

        int max = pow(render_distance * 2, 2);
        int safe_offset = render_distance * 2 * 4;

        while(generated_count < max){
            if(should_stop) return;
            if(HandlePriorityMeshes()) continue;

            while(meshing_indexer.getTotal() < generated_count - safe_offset){
                glm::ivec2 column_position = meshing_indexer.get();
                meshing_indexer.next();

                for(int i = bottom_y;i < top_y;i++){
                    if(should_stop) return;
                    if(HandlePriorityMeshes()) continue;

                    glm::ivec3 chunkPosition = glm::ivec3{column_position.x,i,column_position.y} + around;

                    auto* chunk = terrain.getChunk(chunkPosition);
                    if(!chunk) continue;

                    auto level = calculateSimplificationLevel(around,chunkPosition);

                    mesh_generator.syncGenerateAsyncUploadMesh(chunk, create_mesh(), level);
                }
            }
        }
    });

    service_manager->AddModule("generator", [this](std::atomic<bool>& should_stop){
        if(!game_state || !game_state->world_stream) return;

        auto& terrain = game_state->getTerrain();
        auto& world_stream = *game_state->world_stream;

        SpiralIndexer generation_indexer = {};

        generated_count = 0;

        glm::ivec3 around = glm::ivec3{around_x.load(),around_y.load(),around_z.load()};
        int max = pow(render_distance * 2, 2);
        int safe_offset = render_distance * 2 * 4;

        while(generation_indexer.getTotal() < max + safe_offset)
        {
            if(should_stop) return;

            glm::ivec2 column_position = generation_indexer.get();
            generation_indexer.next();

            for(int i = bottom_y;i < top_y;i++){
                glm::ivec3 chunkPosition = glm::ivec3{column_position.x,i,column_position.y} + around;

                auto* chunk = terrain.getChunk(chunkPosition);
                auto level = calculateSimplificationLevel(around,chunkPosition);

                if(chunk){
                    chunk->current_simplification = level;
                    continue;
                }

                if(world_stream.HasChunkAt(chunkPosition)){
                    terrain.addChunk(chunkPosition, world_stream.Load(chunkPosition));
                    continue;
                }
                chunk = terrain.createEmptyChunk(chunkPosition);
                chunk->current_simplification = level;

                world_generator.generateTerrainChunk(chunk, chunkPosition);
            }

            {
                std::lock_guard lock(loaded_column_mutex);
                loaded_columns.emplace(glm::ivec2{column_position.x + around.x, column_position.y + around.z});
            }
            generated_count++;
        }
    });

    service_manager->AddModule("unloader", [this](std::atomic<bool>& should_stop){
        std::unordered_set<glm::ivec2, IVec2Hash, IVec2Equal> wave_copy;
        {
            std::lock_guard lock(loaded_column_mutex);
            wave_copy = loaded_columns;
        }

        glm::ivec2 center_position = {around_x.load(), around_z.load()};

        int distance = render_distance.load();

        glm::ivec2 min = center_position - distance;
        glm::ivec2 max = center_position + distance;

        for(auto& position: wave_copy){
            if(should_stop) break;
            if(
                position.x > min.x && position.x < max.x &&
                position.y > min.y && position.y < max.y
            ) continue;

            UnloadChunkColumn({position.x, position.y});

            {
                std::lock_guard lock(loaded_column_mutex);
                loaded_columns.erase(position);
            }
        }
    });
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

    around_x = around.x;
    around_y = around.y;
    around_z = around.z;
    this->render_distance = render_distance;

    service_manager->StartAll(true);

    return true;
}

BitField3D::SimplificationLevel TerrainManager::calculateSimplificationLevel(const glm::vec3& around, const glm::vec3& chunkPosition){
    int distance = glm::clamp(glm::distance(glm::vec3(around), glm::vec3(chunkPosition)) / 3.0f, 0.0f, 7.0f);
    return static_cast<BitField3D::SimplificationLevel>(distance - 1);
}

bool TerrainManager::uploadPendingMeshes(){
    return mesh_generator.loadMeshFromQueue(mesh_registry, 10);
}

void TerrainManager::UnloadChunkColumn(const glm::ivec2& position){
    if(!game_state || !game_state->world_stream) return;

    auto& terrain = game_state->getTerrain();

    for(int i = bottom_y;i < top_y;i++){
        glm::ivec3 chunkPosition = glm::ivec3{position.x,i,position.y};

        mesh_registry.removeMesh(chunkPosition);
        game_state->unloadChunk(chunkPosition);
    }
}

void TerrainManager::unloadAll(){
    mesh_registry.clear();
    reset_loader();
    world_generator.getHeightMaps() = std::unordered_map<glm::ivec3, std::unique_ptr<WorldGenerator::Heightmap>, IVec3Hash, IVec3Equal>();
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

    if(service_manager->IsRunning("mesher")) has_priority_meshes = true;
}
#undef regenMesh

void TerrainManager::regenerateChunkMesh(Chunk* chunk){
    if(!service_manager->IsRunning("mesher")) mesh_generator.syncGenerateSyncUploadMesh(chunk, mesh_registry, create_mesh(), BitField3D::NONE);
    else if(!has_priority_meshes){
        priority_mesh_queue[priority_count++] = chunk;
    }
}
