#include <game/world/world_generation.hpp>

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
    trunkBlock.type = BlockType::BirchLog;
    Block leafBlock;
    leafBlock.type = BlockType::BirchLeafBlock;

    generateTree(chunk,x,y,z,trunkBlock,leafBlock);
}

void generateOakTree(Chunk& chunk, int x, int y, int z){
    Block trunkBlock;
    trunkBlock.type = BlockType::OakLog;
    Block leafBlock;
    leafBlock.type = BlockType::LeafBlock;

    generateTree(chunk, x,y,z,trunkBlock,leafBlock);
}

void generateNoTree(Chunk& /*chunk*/, int /*x*/, int /*y*/, int /*z*/){}


std::vector<Biome> biomes = {
    Biome(BlockType::Sand, BlockType::Sand, BlockType::Stone, 0.8f, 1.0f, generateNoTree),
    Biome(BlockType::Grass, BlockType::Dirt, BlockType::Stone, 0.4f, 0.8f, generateOakTree),
    Biome(BlockType::Stone, BlockType::Stone, BlockType::Stone, 0.0f, 0.4f, generateNoTree)
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

std::random_device dev;
std::mt19937 rng(dev());
std::uniform_int_distribution<std::mt19937::result_type> dist6(1,10); // distribution in range [1, 6]

WorldGenerator::WorldGenerator(int seed){
    computeProgram.initialize();
    computeProgram.addShader("shaders/compute/terrain_generation.glsl", GL_COMPUTE_SHADER);
    computeProgram.compile();
    computeProgram.use();

    computeBuffer = std::make_unique<GLPersistentBuffer<uint>>(64 * 64  * (64 / 32) * sizeof(uint), GL_SHADER_STORAGE_BUFFER);

    worldPositionUniformID = glGetUniformLocation(computeProgram.getID(),"worldPosition");
    if(worldPositionUniformID == -1) {
        std::cerr << "Generation program missing world position uniform";
    }

    GLuint uniformBlockIndex = glGetUniformBlockIndex(computeProgram.getID(), "UniformBuffer");
    glUniformBlockBinding(computeProgram.getID(), uniformBlockIndex, 1);


    CHECK_GL_ERROR();

    //noise = std::make_unique<FastNoiseLite>();
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.001f);
    noise.SetFractalOctaves(3);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(seed);
}

static inline float transcribeNoiseValue(float value, float ry){
    value -= std::max(ry / 256, 0.0f);
    value = std::max(0.0f, value);

    return value;
}

static inline void set(DynamicChunkContents* group, int x, int y, int z, BlockType type, bool solid){
    group->getMask(type).set(x,y,z);
    if(solid) group->getSolidField().set(x,y,z);
}

void WorldGenerator::generateTerrainChunk(Chunk* chunk, int chunkX, int chunkY, int chunkZ, size_t size){
    auto start = std::chrono::high_resolution_clock::now();

    std::unique_ptr<DynamicChunkContents> outputDataGroup = std::make_unique<DynamicChunkContents>(size);
    /*
        MAKE SURE THAT ALL THE MASKS EXIST, CRASHES OTHERWISE!
    */
    outputDataGroup->createMask(BlockType::OakLog, size);
    outputDataGroup->createMask(BlockType::Grass, size);
    outputDataGroup->createMask(BlockType::LeafBlock, size);
    outputDataGroup->createMask(BlockType::Dirt, size);
    outputDataGroup->createMask(BlockType::GrassBillboard, size);

    float jump = static_cast<float>(CHUNK_SIZE) / static_cast<float>(size);

    for(int x = 0;x < size;x++) for(int y = 0;y < size;y++) for(int z = 0;z < size;z++){
        float rx = static_cast<float>(x) * jump + chunkX * CHUNK_SIZE;
        float ry = static_cast<float>(y) * jump + chunkY * CHUNK_SIZE;
        float rz = static_cast<float>(z) * jump + chunkZ * CHUNK_SIZE;
        
        float value = transcribeNoiseValue(noise.GetNoise(rx, ry, rz),  ry);
        bool top = transcribeNoiseValue(noise.GetNoise(rx, ry + jump, rz), ry + jump) <= 0.5;

        if(value > 0.5){
            if(top){
                set(outputDataGroup.get(), x, y, z, BlockType::Grass, true);
            }
            else{
                set(outputDataGroup.get(), x, y, z, BlockType::Dirt, true);
            }
            //chunk.setBlock(x,y,z, {top ? BlockType::Grass : BlockType::Stone});

            //if(top && rand() % 30 == 0) generateOakTree(chunk,x,y+1,z);
        }
    }

    chunk->setMainGroup(std::move(outputDataGroup));

    //std::cout << "Generated with mask:" << chunk.getMainGroupAs<64>() << " " << chunk.getMainGroupAs<64>()->masks.size() << std::endl;
    
    /*for(int x = 0;x < CHUNK_SIZE;x++) for(int y = 0;y < CHUNK_SIZE;y++) for(int z = 0;z < CHUNK_SIZE;z++){
        float rx = (float)(x + chunkX * CHUNK_SIZE);
        float ry = (float)(y + chunkY * CHUNK_SIZE);
        float rz = (float)(z + chunkZ * CHUNK_SIZE);

        Block* block = chunk.getBlock(x,y,z);
        Block* upperBlock = chunk.getBlock(x,y + 1,z); 
        if(block->type == BlockType::Stone && upperBlock && upperBlock->type == BlockType::Air){
            chunk.setBlock(x,y,z, {BlockType::Grass});

            if(dist6(rng) == 100) generateOakTree(chunk, x,y,z);
        }
    }*/



      // End time point
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Generated chunk (" << chunkX << "," << chunkY << "," << chunkZ << ") in: " << duration << " microseconds" << std::endl;

}   

bool first = true;
void WorldGenerator::generateTerrainChunkAccelerated(Chunk* chunk, int chunkX, int chunkY, int chunkZ, size_t size){
    auto start = std::chrono::high_resolution_clock::now();

    computeProgram.use();

    glm::vec3 worldPosition = {chunkX * CHUNK_SIZE,chunkY * CHUNK_SIZE,chunkZ * CHUNK_SIZE};
    glUniform3fv(worldPositionUniformID, 1, glm::value_ptr(worldPosition));

    //computeProgram.updateUniforms();
    CHECK_GL_ERROR();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, computeBuffer->getID());


    CHECK_GL_ERROR();
    
    /*
        64 / 32 (division because one uint is 32 bits) by 64 by 64
    */
    glDispatchCompute(64 / 32, 64, 64);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // Wait until the GPU has finished processing (with a timeout to avoid blocking indefinitely)
    GLenum waitReturn;
    do {
        waitReturn = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000); // 1ms timeout
    } while (waitReturn != GL_ALREADY_SIGNALED && waitReturn != GL_CONDITION_SATISFIED);

    glDeleteSync(fence);

    CHECK_GL_ERROR();

    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Generated chunk (" << chunkX << "," << chunkY << "," << chunkZ << ") in: " << duration << " microseconds" << std::endl;

    std::unique_ptr<DynamicChunkContents> outputDataGroup = std::make_unique<DynamicChunkContents>(size);
    /*
        MAKE SURE THAT ALL THE MASKS EXIST, CRASHES OTHERWISE!
    */
    if(first){
        for(int i = 0;i < 10;i++){
            std::cout << std::bitset<64>(reinterpret_cast<uint64_t*>(computeBuffer->data())[i]) << std::endl;
        }
        first = false;
    }
    auto mask = DynamicChunkMask(reinterpret_cast<uint64_t*>(computeBuffer->data()));
    mask.setBlock({BlockType::Grass});

    outputDataGroup->setMask(BlockType::Grass, mask);
    outputDataGroup->setSolidMask(mask);

    chunk->setMainGroup(std::move(outputDataGroup));
}