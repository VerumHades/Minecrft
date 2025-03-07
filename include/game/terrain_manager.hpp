#pragma once 

#include <rendering/chunk_buffer.hpp>
#include <game/world/mesh_generation.hpp>
#include <game/world/terrain.hpp>
#include <game/world/world_generation.hpp>
#include <game/game_state.hpp>
#include <atomic>
#include <indexing.hpp>
#include <thread>

class TerrainManager{
    private:
        ChunkMeshGenerator mesh_generator;
        ChunkMeshRegistry mesh_registry;
        WorldGenerator world_generator;
        
        GameState* game_state = nullptr;

        void generateRegion(glm::ivec3 around, int render_distance);

        std::atomic<bool> loading_region = false;
        std::atomic<bool> stop_loading_region = false;

        std::atomic<bool> generating_meshes = false;
        std::atomic<bool> has_priority_meshes = false;

        std::atomic<bool> generating_world = false;
        std::atomic<bool> stop_mesh_generation = false;

        static const int thread_count = 8;
        std::array<std::atomic<int>, thread_count> generation_left;

        std::array<Chunk*, 4> priority_mesh_queue{};
        int priority_count = 0;
        
        void meshRegion(glm::ivec3 around, int render_distance);
        BitField3D::SimplificationLevel calculateSimplificationLevel(const glm::vec3& around, const glm::vec3& chunkPosition);
        //void loadChunk(const glm::ivec3& position);
        //void unloadChunk(const glm::ivec3& position);

        bool HandlePriorityMeshes();

    public:
        TerrainManager() {}
        // Actually deploys meshes
        void unloadAll(){ mesh_registry.clear(); }

        bool uploadPendingMeshes();
        bool loadRegion(glm::ivec3 around, int render_distance);

        void regenerateChunkMesh(Chunk* chunk);
        void regenerateChunkMesh(Chunk* chunk,glm::vec3 blockCoords);

        void setGameState(GameState* state){
            if(state == nullptr){
                stopMeshGeneration();
                stopRegionLoading();
            }
            game_state = state;
            if(state == nullptr) mesh_generator.setWorld(nullptr);
            else{
                mesh_generator.setWorld(&game_state->getTerrain());
                world_generator.setSeed(game_state->getSeed());
            }
        }

        void stopMeshGeneration() {
            stop_mesh_generation = true;
            while(generating_meshes) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            stop_mesh_generation = false;
        };

        void stopRegionLoading(){
            stop_loading_region = true;
            while(loading_region) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            stop_loading_region = false;
        }

        ChunkMeshRegistry& getMeshRegistry(){ return mesh_registry; }

        std::array<std::atomic<int>, thread_count>& getGenerationCountsLeft(){ return generation_left; }
        const std::atomic<bool>& isGenerating() { return generating_world; };
        WorldGenerator& getWorldGenerator() { return world_generator; }
};