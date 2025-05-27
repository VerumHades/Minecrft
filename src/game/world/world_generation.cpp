#include <game/world/world_generation.hpp>
#include <memory>
#include <random>

#define FNL_IMPL
#include <FastNoiseLite.h>

const auto GetID = BlockRegistry::GetBlockID;

WorldGenerator::WorldGenerator() {
    tree = std::make_shared<Structure>(5, 7, 5);

    BlockID wood_id = GetID("oak_log");
    BlockID leaf_id = GetID("oak_leaves");

    for (int i = 0; i < 5; i++)
        tree->setBlock({2, i, 2}, {wood_id});

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            for (int g = 0; g < 2; g++) {
                if (i == 2 && j == 2 && g == 0)
                    continue;
                tree->setBlock({i, 4 + g, j}, {leaf_id});
            }

    for (int i = 1; i < 4; i++)
        for (int j = 1; j < 4; j++) {
            tree->setBlock({i, 6, j}, {leaf_id});
        }

    BlockID cactus_id = GetID("cactus");

    cactus = std::make_shared<Structure>(1, 3, 1);
    cactus->setBlock({0, 0, 0}, {cactus_id});
    cactus->setBlock({0, 1, 0}, {cactus_id});

    BlockID grass_id = GetID("grass_billboard");

    grass = std::make_shared<Structure>(1, 1, 1);
    grass->setBlock({0, 0, 0}, {grass_id});

    ByteArray array{};

    FileStream stream{};
    stream.Open("resources/structures/tower.structure");
    stream.MoveCursor(0);
    array.LoadFromStream(stream);

    tower = std::make_shared<Structure>(0, 0, 0);
    Serializer::Deserialize<Structure>(*tower.get(), array);

    std::cout << tower->getSize().x << " " << tower->getSize().y << " " << tower->getSize().z << std::endl;
}

void WorldGenerator::SetupBiomeLayers() {
    biomes.clear();
    auto& height_map_layer = AddNoiseLayer("continentalness");
    height_map_layer.noise.SetFrequency(0.003f);

    auto& weirdness_layer = AddNoiseLayer("weirdness");
    weirdness_layer.noise.SetFrequency(0.01f);
    weirdness_layer.noise.SetFractalOctaves(15);
    weirdness_layer.noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    weirdness_layer.noise.SetFractalLacunarity(2.0f);

    auto& errosion_layer = AddNoiseLayer("errosion");
    errosion_layer.noise.SetFrequency(0.01f);

    auto& temperature_layer = AddNoiseLayer("temperature");
    temperature_layer.noise.SetFrequency(0.0005f);
    temperature_layer.noise.SetFractalOctaves(3);
    temperature_layer.noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    temperature_layer.snap_range = 10;

    auto& humidity_layer = AddNoiseLayer("humidity");
    humidity_layer.noise.SetFrequency(0.001f);
    humidity_layer.noise.SetFractalOctaves(3);
    humidity_layer.noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    humidity_layer.snap_range = 10;
    // noise.SetSeed(1985);
    //
    default_biome = std::make_shared<Biome>(Biome{{0, 0, false, false},
                                                  {0, 0, false, false},
                                                  GetID("grass"),
                                                  GetID("stone"),
                                                  GetID("stone"),
                                                  GetID("water"),
                                                  glm::vec3{100, 150, 100},
                                                  {{20, grass}, {0.05, tower}}});

    biomes.push_back(std::make_shared<Biome>(Biome{{0.3, 0.5},
                                                   {0.2, 0.7},
                                                   GetID("grass"),
                                                   GetID("stone"),
                                                   GetID("stone"),
                                                   GetID("water"),
                                                   glm::vec3{200, 200, 200},
                                                   {{20, tree}}}));

    biomes.push_back(std::make_shared<Biome>(Biome{
        {0.7, 1.0}, {0, 0.3}, GetID("volcanic_sand"), GetID("sand_stone"), GetID("stone"), GetID("water"), glm::vec3{76, 70, 50}}));

    biomes.push_back(std::make_shared<Biome>(Biome{{0.5, 0.7},
                                                   {0, 0.3},
                                                   GetID("sand"),
                                                   GetID("sand_stone"),
                                                   GetID("stone"),
                                                   GetID("water"),
                                                   glm::vec3{240, 189, 22},
                                                   {{1, cactus}}}));
}
void WorldGenerator::placeStructure(const glm::ivec3& position, const std::shared_ptr<Structure>& structure) {
    structures.add(position, structure->getSize(), structure);
}

WorldGenerator::NoiseLayer& WorldGenerator::AddNoiseLayer(const std::string& name) {
    auto& layer = noise_layers[name];
    auto& noise = layer.noise;

    static std::uniform_int_distribution<std::size_t> dist(1000, 10000);

    layer.offset.x = static_cast<float>(dist(*offset_random_engine));
    layer.offset.y = static_cast<float>(dist(*offset_random_engine));

    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.001f);

    return layer;
}

float WorldGenerator::GetNoiseValueAt(const glm::vec3& position, const std::string& layer_name) {
    if (!noise_layers.contains(layer_name))
        return 0;

    auto& layer = noise_layers.at(layer_name);

    glm::vec2 pos = glm::vec2(position.x + layer.offset.x, position.z + layer.offset.y) / 10.0f;
    float value   = (layer.noise.GetNoise(pos.x, pos.y) + 1.0) / 2.0;

    if (layer.snap_range != 0)
        value = (floor((value * 100.0f) / static_cast<float>(layer.snap_range)) * static_cast<float>(layer.snap_range)) / 100.0f;

    return value;
}

Image WorldGenerator::createPreview(int width, int height, float step) {
    Image out{width, height, 4};

    int half_width  = width / 2;
    int half_height = height / 2;

    for (int x = -half_width; x < half_width; x++)
        for (int z = -half_height; z < half_height; z++) {
            auto position = glm::vec3{x, 0, z} * step;

            int value        = GetHeightAt(position);
            float normalized = static_cast<float>(value) / 300.0f;
            normalized = 0.5 + (normalized / 2);

            Biome* biome = GetBiomeFor(position);

            glm::vec3 color = biome ? biome->preview_color : glm::vec3{60, 150, 60};

            if (value < water_level) {
                color      = {60, 60, 150};
                normalized = 1.0f;
            }

            auto* pixel = out.getPixel(x + half_width, z + half_height);

            pixel[0] = color.r * normalized;
            pixel[1] = color.g * normalized;
            pixel[2] = color.b * normalized;
            pixel[3] = 255;
        }

    return out;
}

Biome* WorldGenerator::GetBiomeFor(const glm::ivec3& position) {
    auto temperature = GetNoiseValueAt(position, "temperature");
    auto humidity    = GetNoiseValueAt(position, "humidity");

    std::vector<Biome*> candidates{};
    for (auto& biome : biomes) {
        if (!biome->humidity.IsWithin(humidity) || !biome->temperature.IsWithin(temperature))
            continue;
        candidates.push_back(biome.get());
    }

    if (candidates.size() == 0)
        return default_biome.get();
    return candidates[0];
}

int WorldGenerator::GetHeightAt(const glm::vec3 position) {
    float continentalness = GetNoiseValueAt(position, "continentalness");
    float weirdness       = GetNoiseValueAt(position, "weirdness");
    float errosion        = GetNoiseValueAt(position, "errosion");

    float value = pow(continentalness, 2) * pow(pow(weirdness, 4), pow(errosion, 3));

    return value * 300.0f;
}

WorldGenerator::Heightmap& WorldGenerator::getHeightmapFor(glm::ivec3 position_in) {
    static std::shared_mutex mutex;
    auto position = glm::ivec3(position_in.x, 0, position_in.z);

    {
        std::shared_lock lock(mutex);
        if (getHeightMaps().contains(position))
            return *getHeightMaps().at(position);
    }

    std::unique_lock lock(mutex);

    getHeightMaps().emplace(position, std::make_unique<Heightmap>());
    auto& map = getHeightMaps().at(position);

    map->lowest  = INT32_MAX;
    map->highest = INT32_MIN;

    for (int x = 0; x < CHUNK_SIZE; x++)
        for (int z = 0; z < CHUNK_SIZE; z++) {
            glm::ivec3 localPosition = glm::ivec3(x, 0, z) + position * CHUNK_SIZE;

            int value  = GetHeightAt(localPosition);
            auto biome = GetBiomeFor(localPosition);

            static std::uniform_int_distribution<std::size_t> dist(0, 1000000);

            for (auto& structure : biome->structures) {
                float chance_value = (float)dist(*structure_random_engine) / 10000;
                if (chance_value <= structure.spawn_chance && localPosition.y + value > water_level) {
                    auto size = structure.structure->getSize() / 2;
                    placeStructure(localPosition + glm::ivec3{-size.x, value + 1, -size.y}, structure.structure);
                    break;
                }
            }

            map->lowest        = std::min(value, map->lowest);
            map->highest       = std::max(value, map->highest);
            map->heights[x][z] = value;
            map->biomes[x][z]  = biome;
        }

    return *map;
}

void WorldGenerator::prepareHeightMaps(glm::ivec3 around, int distance) {
    for (int i = -distance; i <= distance; i++)
        for (int j = -distance; j <= distance; j++) {
            getHeightmapFor(around + glm::ivec3{i, 0, j});
        }
}

void WorldGenerator::GenerateTerrainChunk(Chunk* chunk, glm::ivec3 position, unsigned int simplification_step) {
    // static const int count = CHUNK_SIZE / ChunkDefinition::size;
    // Pregen surrounding maps to keep structures whole

    for(int i = -1;i <= 1;i++)
    for(int j = -1;j <= 1;j++){
        if(i == 0 && j == 0) continue;
        getHeightmapFor(position + glm::ivec3{i,0,j});
    }

    auto& heightMap = getHeightmapFor(position);

    if (heightMap.lowest - 1 > position.y * CHUNK_SIZE + CHUNK_SIZE) {
        chunk->fill({heightMap.biomes[0][0]->underground_block});
        return;
    }
    if (heightMap.highest + CHUNK_SIZE < position.y * CHUNK_SIZE) {
        return;
    }

    chunk->generated_simplification_step = simplification_step;

    for (int x = 0; x < CHUNK_SIZE; x += simplification_step)
        for (int y = 0; y < CHUNK_SIZE; y += simplification_step)
            for (int z = 0; z < CHUNK_SIZE; z += simplification_step) {
                glm::ivec3 localPosition = glm::ivec3(x, y, z) + position * CHUNK_SIZE;

                auto* biome = heightMap.biomes[x][z];
                auto height = heightMap.heights[x][z];

                auto* structure_region = structures.get(localPosition);
                if (structure_region && localPosition.y > water_level) {
                    glm::ivec3 relative_position = localPosition - structure_region->min;

                    auto* block = structure_region->value->getBlock(relative_position);
                    if (block && block->id != BLOCK_AIR_INDEX) {
                        chunk->setBlock(glm::ivec3(x, y, z), {block->id}, true);
                        continue;
                    }
                }

                if (localPosition.y > height) {
                    if (localPosition.y <= water_level) {
                        chunk->setBlock(glm::ivec3(x, y, z), {biome->water_block}, true);
                        continue;
                    }
                    continue;
                }

                if (localPosition.y == height)
                    chunk->setBlock(glm::ivec3(x, y, z), {biome->surface_block}, true);
                else
                    chunk->setBlock(glm::ivec3(x, y, z), {biome->underground_block}, true);
            }
}

void WorldGenerator::Clear() {
    getHeightMaps().clear();
}

void WorldGenerator::SetSeed(int seed) {
    this->seed = seed;

    offset_random_engine    = std::make_unique<std::mt19937>(seed);
    structure_random_engine = std::make_unique<std::mt19937>(seed);

    SetupBiomeLayers();

    for (auto& [name, layer] : noise_layers)
        layer.noise.SetSeed(seed);
}
