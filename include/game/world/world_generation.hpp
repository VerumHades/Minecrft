#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <optional>
#include <random>
#include <game/blocks.hpp>

#include <memory>
#include <rendering/opengl/shaders.hpp>
#include <rendering/opengl/texture.hpp>

#include <FastNoiseLite.h> 

class Chunk;

class WorldGenerator{
    private:
        FastNoiseLite noise;
        int seed;

        struct ComputeLayer{
            ShaderProgram program;
            std::string name;
            uint worldPositionUniformID;
        };

        std::array<ComputeLayer, 3> compute_layers = {
            ComputeLayer{ShaderProgram("shaders/compute/grass.glsl"), "grass"},
            ComputeLayer{ShaderProgram("shaders/compute/dirt.glsl"), "dirt"},
            ComputeLayer{ShaderProgram("shaders/compute/stone.glsl"), "stone"},
        };

        GLBuffer<uint, GL_SHADER_STORAGE_BUFFER> computeBuffer;
        GLTexture2D noiseTexture;
        BlockRegistry& blockRegistry;

    public:
        WorldGenerator(BlockRegistry&  blockRegistry);

        void generateTerrainChunk(Chunk* chunk, int chunkX, int chunkY, int chunkZ, size_t size);
        
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

#endif