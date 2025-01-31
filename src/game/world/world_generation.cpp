#include <game/world/world_generation.hpp>

#define FNL_IMPL
#include <FastNoiseLite.h>

WorldGenerator::WorldGenerator() {
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.001f);
    noise.SetFractalOctaves(3);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(1984);


}

void WorldGenerator::generateTerrainChunk(Chunk* chunk, glm::ivec3 position){
    
}   