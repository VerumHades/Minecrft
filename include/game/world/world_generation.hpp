#pragma once

#include <optional>
#include <random>
#include <unordered_map>
#include <thread>
#include <FastNoiseLite.h> 
#include <queue>
#include <random>
#include <memory>
#include <mutex>

#include <rendering/opengl/shaders.hpp>
#include <rendering/opengl/texture.hpp>

#include <game/blocks.hpp>
#include <game/world/region_lookup.hpp>
#include <game/structure.hpp>

#include <vec_hash.hpp>
#include <game/world/region_lookup.hpp>

class Chunk;
class Terrain;

class WorldGenerator{
    public:
        struct Heightmap{
            std::array<std::array<int,CHUNK_SIZE>, CHUNK_SIZE> heights;
            int lowest = INT32_MAX;
            int highest = INT32_MIN;
        };

        BlockID stone = BlockRegistry::get().getIndexByName("stone");
        BlockID grass = BlockRegistry::get().getIndexByName("grass");
        BlockID blue_wool = BlockRegistry::get().getIndexByName("blue_wool");    

        bool isChunkSkipable(Chunk* chunk, const glm::ivec3 position);
        Heightmap& getHeightmapFor(glm::ivec3 position);

        std::unordered_map<glm::ivec3, std::unique_ptr<WorldGenerator::Heightmap>, IVec3Hash, IVec3Equal>& getHeightMaps(){
            static std::unordered_map<glm::ivec3, std::unique_ptr<WorldGenerator::Heightmap>, IVec3Hash, IVec3Equal> height_maps{};
            return height_maps;
        }

    private:
        FastNoiseLite noise;
        int seed;

        std::shared_ptr<Structure> tree;
                    
        RegionRegistry<std::shared_ptr<Structure>> structures;
        void placeStructure(const glm::ivec3& position, const std::shared_ptr<Structure>& structure);

        float getNoiseValueAt(const glm::vec3 position);

        const int water_level = 20;

    public:
        WorldGenerator();

        void prepareHeightMaps(glm::ivec3 around, int distance);

        std::thread threadedQueueGeneration(std::queue<Chunk*>& queue, std::atomic<int>& progress_report, std::atomic<bool>& stop);
        void generateTerrainChunk(Chunk* chunk, glm::ivec3 position);

        void setSeed(int seed) {
            noise.SetSeed(seed);
            this->seed = seed;
        }
        
        int getHeightAt(const glm::vec3 position);

        Image createPreview(int width, int height, float step = 1);
};

#include <game/chunk.hpp>
#include <game/world/terrain.hpp>