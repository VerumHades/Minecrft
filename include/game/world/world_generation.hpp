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

class WorldGenerator : public Generator {
  public:
    struct Heightmap {
        std::array<std::array<int, CHUNK_SIZE>, CHUNK_SIZE> heights;
        std::array<std::array<Biome*, CHUNK_SIZE>, CHUNK_SIZE> biomes;
        int lowest = INT32_MAX;
        int highest = INT32_MIN;
    };

    Heightmap& getHeightmapFor(glm::ivec3 position);

    std::unordered_map<glm::ivec3, std::unique_ptr<WorldGenerator::Heightmap>, IVec3Hash, IVec3Equal>& getHeightMaps() {
        static std::unordered_map<glm::ivec3, std::unique_ptr<WorldGenerator::Heightmap>, IVec3Hash, IVec3Equal>
            height_maps{};
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

    Biome* GetBiomeFor(const glm::ivec3& position);

    NoiseLayer& AddNoiseLayer(const std::string& name);
    float GetNoiseValueAt(const glm::vec3& position, const std::string& layer_name);

    int seed;

    std::shared_ptr<Structure> tree;

    RegionRegistry<std::shared_ptr<Structure>> structures;
    void placeStructure(const glm::ivec3& position, const std::shared_ptr<Structure>& structure);

    const int water_level = 80;

    void SetupBiomeLayers();

  public:
    WorldGenerator();
    ~WorldGenerator() {}

    void prepareHeightMaps(glm::ivec3 around, int distance);

    void GenerateTerrainChunk(Chunk* chunk, glm::ivec3 position) override;
    int GetHeightAt(const glm::vec3 position) override;
    void Clear() override;
    void SetSeed(int seed) override;

    Image createPreview(int width, int height, float step = 1);
};

#include <game/chunk.hpp>
#include <game/world/terrain.hpp>
