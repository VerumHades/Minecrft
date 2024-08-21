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
    trunkBlock.type = BlockTypeEnum::BirchLog;
    Block leafBlock;
    leafBlock.type = BlockTypeEnum::BirchLeafBlock;

    generateTree(chunk,x,y,z,trunkBlock,leafBlock);
}

void generateOakTree(Chunk& chunk, int x, int y, int z){
    Block trunkBlock;
    trunkBlock.type = BlockTypeEnum::OakLog;
    Block leafBlock;
    leafBlock.type = BlockTypeEnum::LeafBlock;

    generateTree(chunk, x,y,z,trunkBlock,leafBlock);
}

void generateNoTree(Chunk* chunk, int x, int y, int z){}


std::vector<Biome> biomes = {
    Biome(BlockTypeEnum::Sand, BlockTypeEnum::Sand, BlockTypeEnum::Stone, 0.8f, 1.0f, generateNoTree),
    Biome(BlockTypeEnum::Grass, BlockTypeEnum::Dirt, BlockTypeEnum::Stone, 0.4f, 0.8f, generateOakTree),
    Biome(BlockTypeEnum::Stone, BlockTypeEnum::Stone, BlockTypeEnum::Stone, 0.0f, 0.4f, generateNoTree)
};

const Biome& getBiome(float temperature){
    for(int i = 0;i < biomes.size();i++){
        Biome& biome = biomes[i];
        if(temperature < biome.temperatureLower || temperature > biome.temperatureUpper) continue;

        return biome;
    }

    return biomes[0];
}

float lerp(float a, float b, float f)
{
    return a * (1.0 - f) + (b * f);
}

Chunk generateTerrainChunk(World& world, int chunkX, int chunkY){
    Chunk chunk(world, glm::vec2(chunkX, chunkY));

    // Create and configure noise state
    fnl_state noise = fnlCreateState();
    noise.noise_type = FNL_NOISE_PERLIN;
    noise.frequency = 0.004;

    fnl_state noise2 = fnlCreateState();
    noise2.noise_type = FNL_NOISE_PERLIN;
    noise2.frequency = 0.1;

    fnl_state mountain_areas = fnlCreateState();
    mountain_areas.noise_type = FNL_NOISE_PERLIN;
    mountain_areas.frequency = 0.01;

    fnl_state temperatureNoise = fnlCreateState();
    temperatureNoise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    temperatureNoise.frequency = 0.001;

    int waterLevel = 256 / 3;

    for(int x = 0;x < DEFAULT_CHUNK_SIZE;x++){
        for(int z = 0;z < DEFAULT_CHUNK_SIZE;z++){
            float rx = (float)(x + chunkX * DEFAULT_CHUNK_SIZE);
            float ry = (float)(z + chunkY * DEFAULT_CHUNK_SIZE);

            float main = (fnlGetNoise2D(&noise, rx, ry) + 1) / 2;
            float secondary = (fnlGetNoise2D(&noise2, rx, ry) + 1) / 2;
            //float mountains = (fnlGetNoise2D(&mountain_areas, rx, ry) + 1) / 2;


            float temperature = (fnlGetNoise2D(&temperatureNoise, rx, ry) + 1) / 2;
            temperature =  1.0 - lerp(main,temperature,0.1);

            main = lerp(main, secondary, pow(main,7));
            //value *= perlin(rx / 20.0, ry / 20.0);
        
            int height = floor(256 * main); 

            const Biome& biome = getBiome(temperature);

            for(int y = 0;y < height;y++){
                Block block;
                block.type = BlockTypeEnum::Air;

                if(y + 1 == height && height <= waterLevel){
                    block.type = BlockTypeEnum::Sand;
                }
                else if(y >= waterLevel && y <= waterLevel + 3){
                    block.type = BlockTypeEnum::Sand;
                }
                else if(y + 1 == height && height > waterLevel){
                    block.type = biome.topBlock;
                    if(rand() % 50 == 0) biome.generateTree(chunk,x,height,z);
                }
                else if(y + 3 >= height) block.type = biome.secondaryTopBlock;
                else block.type = biome.undergroundBlock;

                chunk.setBlock(x,y,z, block);
            }

            for(int y = height; y < waterLevel;y++){
                Block block;
                block.type = BlockTypeEnum::BlueWool;

                chunk.setBlock(x,y,z, block);
            }
        }
    }
    
    return chunk;
}