#include <chrono>
#include <game/terrain_manager.hpp>
#include <thread>

TerrainManager::TerrainManager(std::shared_ptr<Generator> generator) : world_generator(generator) {
    service_manager = std::make_unique<Service>();

    service_manager->AddModule("mesher", [this](std::atomic<bool>& should_stop) {
        if (!game_state || !game_state->world_saver)
            return;

        glm::ivec3 around = glm::ivec3{around_x.load(), around_y.load(), around_z.load()};

        SpiralIndexer meshing_indexer = {};
        auto& terrain                 = game_state->GetTerrain();

        size_t max         = pow((render_distance + 1) * 2, 2);
        int safe_offset = 3;

        while (generated_count < max) {
            if (should_stop)
                return;

            safe_offset = sqrt(generated_count.load()) * 2;
            while (meshing_indexer.getTotal() < (((size_t)safe_offset < generated_count) ? (generated_count - safe_offset) : 0)) {
                glm::ivec2 column_position = meshing_indexer.get();
                meshing_indexer.next();

                for (int i = bottom_y; i < top_y; i++) {
                    if (should_stop)
                        return;

                    glm::ivec3 chunkPosition = glm::ivec3{column_position.x, i, column_position.y} + around;

                    auto* chunk = terrain.getChunk(chunkPosition);
                    if (!chunk)
                        continue;

                    auto level = calculateSimplificationLevel(around, chunkPosition);
                    
                    mesh_generator.syncGenerateAsyncUploadMesh(chunk, create_mesh(), level);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    service_manager->AddModule("generator", [this](std::atomic<bool>& should_stop) {
        if (!game_state || !game_state->world_saver)
            return;

        auto& terrain     = game_state->GetTerrain();
        auto& world_saver = *game_state->world_saver;

        SpiralIndexer generation_indexer = {};

        glm::ivec3 around = glm::ivec3{around_x.load(), around_y.load(), around_z.load()};
        int max           = pow(render_distance * 2, 2);
        int safe_offset   = render_distance * 2 * 4;

        while (generation_indexer.getTotal() < static_cast<size_t>(max + safe_offset)) {
            if (should_stop)
                return;

            glm::ivec2 column_position = generation_indexer.get();
            generation_indexer.next();

            for (int i = bottom_y; i < top_y; i++) {
                glm::ivec3 chunkPosition = glm::ivec3{column_position.x, i, column_position.y} + around;

                auto* chunk = terrain.getChunk(chunkPosition);
                auto level  = calculateSimplificationLevel(around, chunkPosition);
                auto step = calculateGenerationStep(around, chunkPosition);

                if(chunk && chunk->generated_simplification_step != step && chunk->generated_simplification_step != 1){
                    terrain.takeChunk(chunkPosition);
                }
                else if (chunk) {
                    chunk->current_simplification = level;
                    //if(chunk->generated_simplification_step != step) 
                    //    world_generator->GenerateTerrainChunk(chunk, chunkPosition, step);

                    continue;
                }

                if (world_saver.HasChunkAt(chunkPosition)) {
                    auto loaded = world_saver.Load(chunkPosition);
                    if(loaded) {
                        terrain.addChunk(chunkPosition, std::move(loaded));
                        continue;
                    }
                }
                auto uchunk = std::make_unique<Chunk>();
                uchunk->current_simplification = level;
                uchunk->setWorldPosition(chunkPosition);

                world_generator->GenerateTerrainChunk(uchunk.get(), chunkPosition, step);
                terrain.addChunk(chunkPosition, std::move(uchunk));
            }

            {
                std::lock_guard lock(loaded_column_mutex);
                loaded_columns.emplace(glm::ivec2{column_position.x + around.x, column_position.y + around.z});
            }
            generated_count++;
        }
    });

    service_manager->AddModule("unloader", [this](std::atomic<bool>& should_stop) {
        std::unordered_set<glm::ivec2, IVec2Hash, IVec2Equal> wave_copy;
        {
            std::lock_guard lock(loaded_column_mutex);
            wave_copy = loaded_columns;
        }

        glm::ivec2 center_position = {around_x.load(), around_z.load()};

        int distance = render_distance.load();

        glm::ivec2 min = center_position - distance;
        glm::ivec2 max = center_position + distance;

        for (auto& position : wave_copy) {
            if (should_stop)
                break;
            if (position.x > min.x && position.x < max.x && position.y > min.y && position.y < max.y)
                continue;
            
            UnloadChunkColumn({position.x, position.y});

            {
                std::lock_guard lock(loaded_column_mutex);
                loaded_columns.erase(position);
            }
        }
    });
}

bool TerrainManager::loadRegion(glm::ivec3 around, int render_distance) {
    if (!game_state || !game_state->world_saver)
        return false;

    service_manager->StopAll();

    {
        std::lock_guard lock(loaded_column_mutex);
        loaded_columns.clear();
    }

    mesh_generator.clear();
    generated_count = 0;

    around_x              = around.x;
    around_y              = around.y;
    around_z              = around.z;
    this->render_distance = render_distance;

    //std::cout << "Loading region: " << around.x << " " << around.y << " " << around.z  << " " << render_distance <<  std::endl;
    

    /*if(mesh_loader->DrawFailed()){
        service_manager->StopAll();

        create_mesh = []() { return std::make_unique<PooledMesh>(); };
        reset_loader = [this]() {
            mesh_loader = std::make_unique<PooledMeshLoader>();
            mesh_registry.SetMeshLoader(mesh_loader.get());
        };

        reset_loader();
    }*/
    service_manager->StartAll();

    return true;
}

BitField3D::SimplificationLevel TerrainManager::calculateSimplificationLevel(const glm::vec3& around, const glm::vec3& chunkPosition) {
    int distance = glm::clamp(glm::distance(glm::vec2(around.x,around.z), glm::vec2(chunkPosition.x,chunkPosition.z)) / 3.0f, 0.0f, 7.0f);
    return static_cast<BitField3D::SimplificationLevel>(distance - 1);
}
uint TerrainManager::calculateGenerationStep(const glm::vec3& around, const glm::vec3& chunkPosition){
    int distance = glm::clamp(glm::distance(glm::vec2(around.x,around.z), glm::vec2(chunkPosition.x,chunkPosition.z)) / 3.0f, 0.0f, 6.0f);
    return pow(2, distance);
}

void TerrainManager::UnloadChunkColumn(const glm::ivec2& position) {
    if (!game_state || !game_state->world_saver)
        return;

    for (int i = bottom_y; i < top_y; i++) {
        glm::ivec3 chunkPosition = glm::ivec3{position.x, i, position.y};

        {
            std::lock_guard lock(unload_queue_mutex);
            unload_queue.push(chunkPosition);
        }
        //mesh_registry.removeMesh(chunkPosition);
        game_state->unloadChunk(chunkPosition);
    }
}

std::optional<glm::ivec3> TerrainManager::nextUnloadPosition(){
    std::lock_guard lock(unload_queue_mutex);
    if(unload_queue.empty())
        return std::nullopt;
    
    auto result = unload_queue.front();
    unload_queue.pop();
    return result;
}
void TerrainManager::unloadAll() {
    service_manager->StopAll();
    world_generator->Clear();
    
    should_mesh_loader_reset = true;
}


void TerrainManager::setGameState(std::shared_ptr<GameState> state) {
    unloadAll();

    game_state = state;
    if (!state)
        mesh_generator.setWorld(nullptr);
    else {
        mesh_generator.setWorld(&game_state->GetTerrain());
        world_generator->SetSeed(game_state->getSeed());
    }
}
