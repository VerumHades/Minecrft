#pragma once 

#include <rendering/chunk_buffer.hpp>
#include <game/world/mesh_generation.hpp>
#include <game/world/terrain.hpp>
#include <game/world/world_generation.hpp>
#include <game/game_state.hpp>
#include <atomic>
#include <indexing.hpp>

class TerrainManager{
    private:
        ChunkMeshGenerator mesh_generator;
        ChunkMeshRegistry mesh_registry;
        WorldGenerator world_generator;
        
        GameState* game_state = nullptr;

        void generateRegion(glm::ivec3 around, int render_distance);

        std::atomic<bool> generating_meshes = false;
        std::atomic<Chunk*> priority = nullptr;
        void meshRegion(glm::ivec3 around, int render_distance);
        
        //void loadChunk(const glm::ivec3& position);
        //void unloadChunk(const glm::ivec3& position);

    public:
        TerrainManager(): mesh_registry(12) {}
        // Actually deploys meshes
        void unloadAll(){ mesh_registry.clear(); }

        void uploadPendingMeshes();
        void loadRegion(glm::ivec3 around, int render_distance);

        void regenerateChunkMesh(Chunk* chunk);
        void regenerateChunkMesh(Chunk* chunk,glm::vec3 blockCoords);

        void setGameState(GameState* state){
            game_state = state;
            if(state == nullptr) mesh_generator.setWorld(nullptr);
            else mesh_generator.setWorld(&game_state->getTerrain());
        }

        ChunkMeshRegistry& getMeshRegistry(){ return mesh_registry; }
};