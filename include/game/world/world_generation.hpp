#pragma once

#include <optional>
#include <random>
#include <game/blocks.hpp>

#include <memory>
#include <rendering/opengl/shaders.hpp>
#include <rendering/opengl/texture.hpp>

#include <parsing/source_template.hpp>

#include <FastNoiseLite.h> 

class Chunk;
class World;

struct ComputeLayer{
    std::shared_ptr<ShaderProgram> program;
    std::string name;
    int worldPositionUniformID;

    static ComputeLayer FromTemplate(std::string name, SourceTemplate& template_, std::vector<SourceTemplate::TagValue> values);
};

struct GenerationProfile{
    glm::ivec3 work_group_size;
    glm::ivec3 work_groups;

    std::vector<ComputeLayer> compute_layers = {};
};

class AcceleratedChunkGenerator{
    private:
        FastNoiseLite& noise;

        SourceTemplate base_template = SourceTemplate::fromFile("resources/shaders/compute/base.glsl_template");

        std::vector<ComputeLayer> compute_layers = {};
        GLBuffer<uint, GL_SHADER_STORAGE_BUFFER> computeBuffer;
        GLTexture2D noiseTexture;

    public:
        AcceleratedChunkGenerator(FastNoiseLite& noise);
        
        void addComputeLayer(std::string condition, std::string name);
        void generateChunk(Chunk* chunk, glm::ivec3 chunkPosition);
};

class RegionChunkGenerator{
    private:
        FastNoiseLite& noise;

        SourceTemplate base_region_template  = SourceTemplate::fromFile("resources/shaders/compute/region_base.glsl_template");

        GenerationProfile base_profile;

        GLBuffer<uint, GL_SHADER_STORAGE_BUFFER> computeBuffer;
        GLTexture2D noiseTexture;
        
        GenerationProfile createProfile(glm::ivec3 work_group_size, glm::ivec3 work_groups, std::vector<std::string> conditions);

    public:
        RegionChunkGenerator(FastNoiseLite& noise);
        
        // Profiles that contain invalid sizes will cause this to break
        void generate(World& world, glm::ivec3 region_start, GenerationProfile& profile);
        void generateWithBaseProfile(World& world, glm::ivec3 region_start);
};

class WorldGenerator{
    private:
        FastNoiseLite noise;
        int seed;

        std::unique_ptr<AcceleratedChunkGenerator> accelerated_generator;
        std::unique_ptr<RegionChunkGenerator> region_generator;

    public:
        WorldGenerator();

        void generateTerrainChunk(Chunk* chunk, int chunkX, int chunkY, int chunkZ, size_t size);
        
        /*
            A gpu accelerated generation function
        */
        void generateTerrainChunkAccelerated(Chunk* chunk, glm::ivec3 chunkPosition);


        /*
            A gpu accelerated function that generates chunks in a batch
        */
        void generateChunkRegion(World& world, glm::ivec3 start);

        void setSeed(int seed) {
            noise.SetSeed(seed);
            this->seed = seed;
        }
};

#include <game/chunk.hpp>
#include <game/world/world.hpp>