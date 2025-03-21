#pragma once 

#include <rendering/region_culler.hpp>
#include <rendering/vertex_pooling/pooled_mesh.hpp>
#include <rendering/instanced_mesh.hpp>

#include <game/world/mesh_generation.hpp>
#include <game/world/terrain.hpp>
#include <game/world/world_generation.hpp>
#include <game/game_state.hpp>


#include <atomic>
#include <indexing.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>

class TerrainManager{
    private:
        ChunkMeshGenerator mesh_generator;
        
        RegionCuller mesh_registry;
        std::unique_ptr<MeshLoaderInterface> mesh_loader;

        WorldGenerator world_generator;
        
        GameState* game_state = nullptr;

        static const int thread_count = 8;
        std::array<std::atomic<int>, thread_count> generation_left;

        std::atomic<bool> has_priority_meshes = false;
        std::array<Chunk*, 4> priority_mesh_queue{};
        int priority_count = 0;


        int bottom_y = -3;
        int top_y    =  3;
        
        BitField3D::SimplificationLevel calculateSimplificationLevel(const glm::vec3& around, const glm::vec3& chunkPosition);
        //void loadChunk(const glm::ivec3& position);
        //void unloadChunk(const glm::ivec3& position);

        std::atomic<bool> stop_generating_region = false;
        std::atomic<bool> generating_region = false;
        std::atomic<int> generated_count = 0;
        bool GenerateRegion(const glm::ivec3& around, int render_distance);

        void StopGeneratingRegion(){
            if(!generating_region) return;
            
            stop_generating_region = true;
            while(generating_region) {}
            stop_generating_region = false;
        }

        std::atomic<bool> stop_meshing_region = false;
        std::atomic<bool> meshing_region = false;
        bool MeshRegion(const glm::ivec3& around, int render_distance);

        void StopMeshingRegion(){
            if(!meshing_region) return;
            stop_meshing_region = true;
            while(meshing_region) {}
            stop_meshing_region = false;
        }

        bool HandlePriorityMeshes();

        std::function<std::unique_ptr<MeshInterface>()> create_mesh = [](){ return std::make_unique<InstancedMesh>(); };
        std::function<void(void)> reset_loader = [this](){
            mesh_loader = std::make_unique<InstancedMeshLoader>();
            mesh_registry.SetMeshLoader(mesh_loader.get());
        };

        std::mutex loaded_column_mutex;
        std::unordered_set<glm::ivec3, IVec3Hash, IVec3Equal> loaded_columns;
        
        void UnloadChunkColumn(const glm::ivec2& position);

        int unloader_wave_time = 10000; // 30 seconds for each unloader pass
        std::atomic<bool> stop_unloader = false;
        std::atomic<bool> unloader_running = false;

        std::atomic<int> unloader_distance = 10;
        std::atomic<int> unloader_x = 0;
        std::atomic<int> unloader_z = 0;

        void LaunchChunkUnloader();


    public:
        TerrainManager();
        ~TerrainManager(){
            StopGeneratingRegion();
            StopMeshingRegion();

            stop_unloader = true;
            while(unloader_running) {}
        }
        // Actually deploys meshes
        void unloadAll();

        bool uploadPendingMeshes();
        bool loadRegion(glm::ivec3 around, int render_distance);

        void regenerateChunkMesh(Chunk* chunk);
        void regenerateChunkMesh(Chunk* chunk,glm::vec3 blockCoords);

        void setGameState(GameState* state){
            if(state == nullptr){
                StopGeneratingRegion();
                StopMeshingRegion();
            }

            game_state = state;
            if(state == nullptr) mesh_generator.setWorld(nullptr);
            else{
                mesh_generator.setWorld(&game_state->getTerrain());
                world_generator.setSeed(game_state->getSeed());
            }
        }


        RegionCuller& getMeshRegistry(){ return mesh_registry; }

        std::array<std::atomic<int>, thread_count>& getGenerationCountsLeft(){ return generation_left; }
        WorldGenerator& getWorldGenerator() { return world_generator; }
};