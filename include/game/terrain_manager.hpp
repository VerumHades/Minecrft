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

/**
 * @brief A class that maintains terrain and mesh generation and oversees its ingame managment
 * 
 */
class TerrainManager {
  private:
    ChunkMeshGenerator mesh_generator;
    std::shared_ptr<Generator> world_generator;

    std::shared_ptr<GameState> game_state = nullptr;

    int bottom_y = -3;
    int top_y    = 3;

    BitField3D::SimplificationLevel calculateSimplificationLevel(const glm::vec3& around, const glm::vec3& chunkPosition);
    uint calculateGenerationStep(const glm::vec3& around, const glm::vec3& chunkPosition);

    std::mutex loaded_column_mutex;
    std::unordered_set<glm::ivec2, IVec2Hash, IVec2Equal> loaded_columns;

    std::mutex unload_queue_mutex;
    std::queue<glm::ivec3> unload_queue;

    std::atomic<int> around_x        = 0;
    std::atomic<int> around_y        = 0;
    std::atomic<int> around_z        = 0;
    std::atomic<int> render_distance = 0;
    
    std::atomic<bool> should_mesh_loader_reset = 0;

    std::atomic<size_t> generated_count = 0;

    void UnloadChunkColumn(const glm::ivec2& position);

    std::function<std::unique_ptr<MeshInterface>()> create_mesh = []() { return std::make_unique<InstancedMesh>(); };
    std::unique_ptr<Service> service_manager;

  public:
    TerrainManager(std::shared_ptr<Generator> world_generator);

	/**
	 * @brief Unload everything, chunks and meshes
	 * 
	 */
    void unloadAll();

	/**
	 * @brief Load a region around a set point, calling it again starts loading new region instead
	 * 
	 * @param around 
	 * @param render_distance 
	 * @return true 
	 * @return false 
	 */
    bool loadRegion(glm::ivec3 around, int render_distance);

    void setGameState(std::shared_ptr<GameState> state);

    ChunkMeshGenerator& getMeshGenerator(){
        return mesh_generator;
    }

    std::optional<glm::ivec3> nextUnloadPosition();

    bool shouldMeshLoaderReset(){
        return should_mesh_loader_reset;
    }
    void meshLoaderReset(){
        should_mesh_loader_reset = false;
    }

};
