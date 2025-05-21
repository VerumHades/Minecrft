#pragma once

#include <rendering/region_culler.hpp>
#include <rendering/vertex_pooling/pooled_mesh.hpp>
#include <rendering/instanced_mesh.hpp>

#include <game/world/mesh_generation.hpp>
#include <game/world/terrain.hpp>
#include <game/world/generator.hpp>
#include <game/game_state.hpp>

#include <structure/service.hpp>

#include <atomic>
#include <indexing.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>

class TerrainManager {
  private:
    ChunkMeshGenerator mesh_generator;
    std::shared_ptr<Generator> world_generator;

    RegionCuller mesh_registry;
    std::unique_ptr<MeshLoaderInterface> mesh_loader;

    GameState* game_state = nullptr;

    int bottom_y = -3;
    int top_y    = 3;

    BitField3D::SimplificationLevel calculateSimplificationLevel(const glm::vec3& around, const glm::vec3& chunkPosition);

    std::unique_ptr<Service> service_manager;

    std::mutex loaded_column_mutex;
    std::unordered_set<glm::ivec2, IVec2Hash, IVec2Equal> loaded_columns;

    std::atomic<int> around_x        = 0;
    std::atomic<int> around_y        = 0;
    std::atomic<int> around_z        = 0;
    std::atomic<int> render_distance = 0;

    std::atomic<size_t> generated_count = 0;

    void UnloadChunkColumn(const glm::ivec2& position);

    std::function<std::unique_ptr<MeshInterface>()> create_mesh = []() { return std::make_unique<InstancedMesh>(); };
    std::function<void(void)> reset_loader                      = [this]() {
        mesh_loader = std::make_unique<InstancedMeshLoader>();
        mesh_registry.SetMeshLoader(mesh_loader.get());
    };

  public:
    TerrainManager(std::shared_ptr<Generator> world_generator);

    void unloadAll();

    bool uploadPendingMeshes();
    bool loadRegion(glm::ivec3 around, int render_distance);

    void regenerateChunkMesh(Chunk* chunk);
    void regenerateChunkMesh(Chunk* chunk, glm::vec3 blockCoords);

    void setGameState(GameState* state);

    RegionCuller& getMeshRegistry() {
        return mesh_registry;
    }
};
