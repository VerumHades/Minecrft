#pragma once

#include <FastNoiseLite.h>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <thread>
#include <unordered_map>

#include <rendering/opengl/shaders.hpp>
#include <rendering/opengl/texture.hpp>

#include <structure/interval.hpp>

#include <vec_hash.hpp>

#include <game/blocks.hpp>
#include <game/structure.hpp>
#include <game/world/biomes/biome.hpp>
#include <game/world/generator.hpp>
#include <game/world/region_lookup.hpp>

class Chunk;
class Terrain;

/**
 * @brief A world generation implementation with perlin 2D noise and biome generation
 * 
 */
class WorldGenerator : public Generator {
  public:
    struct Heightmap {
        std::array<std::array<int, CHUNK_SIZE>, CHUNK_SIZE> heights;
        std::array<std::array<Biome*, CHUNK_SIZE>, CHUNK_SIZE> biomes;
        int lowest  = INT32_MAX;
        int highest = INT32_MIN;
    };

    Heightmap& getHeightmapFor(glm::ivec3 position);

    std::unordered_map<glm::ivec3, std::unique_ptr<WorldGenerator::Heightmap>, IVec3Hash, IVec3Equal>& getHeightMaps() {
        static std::unordered_map<glm::ivec3, std::unique_ptr<WorldGenerator::Heightmap>, IVec3Hash, IVec3Equal> height_maps{};
        return height_maps;
    }

  private:
    struct NoiseLayer {
        FastNoiseLite noise;
        glm::vec2 offset;
        int snap_range = 0;
    };

    std::unique_ptr<std::mt19937> offset_random_engine;
    std::unique_ptr<std::mt19937> structure_random_engine;

    std::unordered_map<std::string, NoiseLayer> noise_layers;
    std::vector<std::shared_ptr<Biome>> biomes;

    std::shared_ptr<Biome> default_biome;

    /**
     * @brief Get relevant biome for a given position
     * 
     * @param position 
     * @return Biome* 
     */
    Biome* GetBiomeFor(const glm::ivec3& position);

    NoiseLayer& AddNoiseLayer(const std::string& name);
    float GetNoiseValueAt(const glm::vec3& position, const std::string& layer_name);

    int seed;

    std::shared_ptr<Structure> tree;
    std::shared_ptr<Structure> grass;
    std::shared_ptr<Structure> cactus;
    std::shared_ptr<Structure> tower;

    RegionRegistry<std::shared_ptr<Structure>> structures;
    void placeStructure(const glm::ivec3& position, const std::shared_ptr<Structure>& structure);

    const int water_level = 0;

    void SetupBiomeLayers();

  public:
    WorldGenerator();
    ~WorldGenerator() {}

    /**
     * @brief Pregenerates heightmaps for a given range and position
     * 
     * @param around 
     * @param distance 
     */
    void prepareHeightMaps(glm::ivec3 around, int distance);

    /**
     * @brief Generates a chunk for a given position
     * 
     * @param chunk 
     * @param position a chunk position, not a world position
     */
    void GenerateTerrainChunk(Chunk* chunk, glm::ivec3 position) override;

    /**
     * @brief Returns noise height at a given point
     * 
     * @param position 
     * @return int 
     */
    int GetHeightAt(const glm::vec3 position) override;

    void Clear() override;
    void SetSeed(int seed) override;

    /**
     * @brief Generates a topdown preview of the world as an image
     * 
     * @param width 
     * @param height 
     * @param step block to pixel scale
     * @return Image 
     */
    Image createPreview(int width, int height, float step = 1);
};

#include <game/chunk.hpp>
#include <game/world/terrain.hpp>
