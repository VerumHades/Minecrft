#include <worldgen/worldgen.hpp>
#define FNL_IMPL
#include <FastNoiseLite.h>

static inline void generateTree(Chunk& chunk, int x, int y, int z, Block trunkBlock, Block leafBlock){
    int trunkHeight = rand() % 5 + 5;

    for(int g = 0;g < 2;g++){
        for(int i = -2; i <= 2;i++){
            for(int j = -2;j <= 2;j++){
                if(j == 0 && i == 0 && g == 0) continue;
                chunk.setBlock(x+i,y+trunkHeight-1+g,z+j,leafBlock);       
            }
        }
    }

    for(int i = -1; i <= 1;i++){
        for(int j = -1;j <= 1;j++){
            chunk.setBlock(x+i,y+trunkHeight+1,z+j,leafBlock);       
        }
    }

    for(int i = -1; i <= 1;i++){
        for(int j = -1;j <= 1;j++){
            if(i != 0 && j != 0) continue;
            chunk.setBlock(x+i,y+trunkHeight+2,z+j,leafBlock);       
        }
    }

    for(int i = 0; i < trunkHeight;i++) chunk.setBlock(x,y+i,z, trunkBlock);
}

void generateBirchTree(Chunk& chunk, int x, int y, int z){
    Block trunkBlock;
    trunkBlock.type = BlockTypes::BirchLog;
    Block leafBlock;
    leafBlock.type = BlockTypes::BirchLeafBlock;

    generateTree(chunk,x,y,z,trunkBlock,leafBlock);
}

void generateOakTree(Chunk& chunk, int x, int y, int z){
    Block trunkBlock;
    trunkBlock.type = BlockTypes::OakLog;
    Block leafBlock;
    leafBlock.type = BlockTypes::LeafBlock;

    generateTree(chunk, x,y,z,trunkBlock,leafBlock);
}

void generateNoTree(Chunk& /*chunk*/, int /*x*/, int /*y*/, int /*z*/){}


std::vector<Biome> biomes = {
    Biome(BlockTypes::Sand, BlockTypes::Sand, BlockTypes::Stone, 0.8f, 1.0f, generateNoTree),
    Biome(BlockTypes::Grass, BlockTypes::Dirt, BlockTypes::Stone, 0.4f, 0.8f, generateOakTree),
    Biome(BlockTypes::Stone, BlockTypes::Stone, BlockTypes::Stone, 0.0f, 0.4f, generateNoTree)
};

const Biome& getBiome(float temperature){
    for(int i = 0;i < biomes.size();i++){
        Biome& biome = biomes[i];
        if(temperature < biome.temperatureLower || temperature > biome.temperatureUpper) continue;

        return biome;
    }

    return biomes[0];
}

/*float lerp(float a, float b, float f)
{
    return a * (1.0f - f) + (b * f);
}*/

void generateTerrainChunk(Chunk& chunk, int chunkX, int chunkY, int chunkZ){
    // Create and configure noise state
    fnl_state noise = fnlCreateState();
    noise.noise_type = FNL_NOISE_PERLIN;
    noise.frequency = 0.004f;

    for(int x = 0;x < CHUNK_SIZE;x++) for(int y = 0;y < CHUNK_SIZE;y++) for(int z = 0;z < CHUNK_SIZE;z++){
        float rx = (float)(x + chunkX * CHUNK_SIZE);
        float ry = (float)(y + chunkY * CHUNK_SIZE);
        float rz = (float)(z + chunkZ * CHUNK_SIZE);
            
        float value = fnlGetNoise3D(&noise, rx, ry, rz);

        value -= ry / 256;
        value = std::max(0.0f, value);

        if(value > 0.5){
            chunk.setBlock(x,y,z, {BlockTypes::Stone});
        }
    }
 
}