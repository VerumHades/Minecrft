#include <worldgen/worldgen.hpp>
#define FNL_IMPL
#include <FastNoiseLite.hpp>

static inline void generateTree(Chunk* chunk, int x, int y, int z, Block trunkBlock, Block leafBlock){
    int trunkHeight = rand() % 5 + 5;

    for(int g = 0;g < 2;g++){
        for(int i = -2; i <= 2;i++){
            for(int j = -2;j <= 2;j++){
                if(j == 0 && i == 0 && g == 0) continue;
                setChunkBlock(chunk,x+i,y+trunkHeight-1+g,z+j,leafBlock);       
            }
        }
    }

    for(int i = -1; i <= 1;i++){
        for(int j = -1;j <= 1;j++){
            setChunkBlock(chunk,x+i,y+trunkHeight+1,z+j,leafBlock);       
        }
    }

    for(int i = -1; i <= 1;i++){
        for(int j = -1;j <= 1;j++){
            if(i != 0 && j != 0) continue;
            setChunkBlock(chunk,x+i,y+trunkHeight+2,z+j,leafBlock);       
        }
    }

    for(int i = 0; i < trunkHeight;i++) setChunkBlock(chunk,x,y+i,z, trunkBlock);
}

void generateBirchTree(Chunk* chunk, int x, int y, int z){
    Block trunkBlock;
    trunkBlock.typeIndex = 7;
    Block leafBlock;
    leafBlock.typeIndex = 6;

    generateTree(chunk,x,y,z,trunkBlock,leafBlock);
}

void generateOakTree(Chunk* chunk, int x, int y, int z){
    Block trunkBlock;
    trunkBlock.typeIndex = 5;
    Block leafBlock;
    leafBlock.typeIndex = 4;

    generateTree(chunk,x,y,z,trunkBlock,leafBlock);
}

void generateNoTree(Chunk* chunk, int x, int y, int z){}


Biome biomes[] = {
    {
        .generateTree = generateNoTree,
        .topBlock = 9,
        .secondaryTopBlock = 9,
        .undergroundBlock = 3,
        .temperature_lower = 0.8,
        .temperature_upper = 1.0
    },
    {
        .generateTree = generateOakTree,
        .topBlock = 1,
        .secondaryTopBlock = 2,
        .undergroundBlock = 3,
        .temperature_lower = 0.4,
        .temperature_upper = 0.8
    },
    {
        .generateTree = generateNoTree,
        .topBlock = 3,
        .secondaryTopBlock = 3,
        .undergroundBlock = 3,
        .temperature_lower = 0.0,
        .temperature_upper = 0.4
    },
};

size_t biomesTotal = arrayLen(biomes);

Biome* getBiome(float temperature){
    for(int i = 0;i < biomesTotal;i++){
        Biome* biome = &biomes[i];
        if(temperature < biome->temperature_lower || temperature > biome->temperature_upper) continue;

        return biome;
    }

    return NULL;
}

float lerp(float a, float b, float f)
{
    return a * (1.0 - f) + (b * f);
}

Chunk* generateTerrainChunk(World* world, int chunkX, int chunkZ){
    Chunk* chunk = generateEmptyChunk(world);

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
            float rz = (float)(z + chunkZ * DEFAULT_CHUNK_SIZE);

            float main = (fnlGetNoise2D(&noise, rx, rz) + 1) / 2;
            float secondary = (fnlGetNoise2D(&noise2, rx, rz) + 1) / 2;
            //float mountains = (fnlGetNoise2D(&mountain_areas, rx, rz) + 1) / 2;


            float temperature = (fnlGetNoise2D(&temperatureNoise, rx, rz) + 1) / 2;
            temperature =  1.0 - lerp(main,temperature,0.1);

            main = lerp(main, secondary, pow(main,7));
            //value *= perlin(rx / 20.0, rz / 20.0);
        
            int height = floor(256 * main); 

            Biome* biome = getBiome(temperature);
            if(biome == NULL) biome = &biomes[0];

            for(int y = 0;y < height;y++){
                Block block;
                block.typeIndex = 0;

                if(y + 1 == height && height <= waterLevel){
                    block.typeIndex = 9;
                }
                else if(y >= waterLevel && y <= waterLevel + 3){
                    block.typeIndex = 9;
                }
                else if(y + 1 == height && height > waterLevel){
                    block.typeIndex = biome->topBlock;
                    if(rand() % 50 == 0) biome->generateTree(chunk,x,height,z);
                }
                else if(y + 3 >= height) block.typeIndex = biome->secondaryTopBlock;
                else block.typeIndex = biome->undergroundBlock;

                setChunkBlock(chunk,x,y,z, block);
            }

            for(int y = height; y < waterLevel;y++){
                Block block;
                block.typeIndex = 8;

                setChunkBlock(chunk,x,y,z, block);
            }
        }
    }
    
    return chunk;
}