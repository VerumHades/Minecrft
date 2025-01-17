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

class AcceleratedChunkGenerator{
    private:
        FastNoiseLite& noise;
        
        const int noise_width = 1024;
        const int noise_height = 1024;
        
        Image noise_img{noise_width,noise_height,1};

        SourceTemplate base_template = SourceTemplate::fromFile("resources/shaders/compute/base.glsl_template");

        std::vector<ComputeLayer> compute_layers = {};
        GLBuffer<uint, GL_SHADER_STORAGE_BUFFER> computeBuffer;
        GLTexture2D noiseTexture;

    public:
        AcceleratedChunkGenerator(FastNoiseLite& noise);
        
        void addComputeLayer(std::string condition, std::string name);
        void generateChunk(Chunk* chunk, glm::ivec3 chunkPosition);
};


class WorldGenerator{
    private:
        FastNoiseLite noise;
        int seed;

        std::unique_ptr<AcceleratedChunkGenerator> accelerated_generator;
        
    public:
        WorldGenerator();

        void generateTerrainChunk(Chunk* chunk, glm::ivec3 position);
        
        /*
            A gpu accelerated generation function
        */
        void generateTerrainChunkAccelerated(Chunk* chunk, glm::ivec3 chunkPosition);

        void setSeed(int seed) {
            noise.SetSeed(seed);
            this->seed = seed;
        }
};

#include <game/chunk.hpp>
#include <game/world/world.hpp>