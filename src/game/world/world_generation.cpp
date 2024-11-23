#include <game/world/world_generation.hpp>

#define FNL_IMPL
#include <FastNoiseLite.h>


std::vector<Biome> biomes = {
    Biome(BlockType::Sand, BlockType::Sand, BlockType::Stone, 0.8f, 1.0f),
    Biome(BlockType::Grass, BlockType::Dirt, BlockType::Stone, 0.4f, 0.8f),
    Biome(BlockType::Stone, BlockType::Stone, BlockType::Stone, 0.0f, 0.4f)
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


    //CHECK_GL_ERROR();

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

void WorldGenerator::generateTerrainChunk(Chunk* chunk, int chunkX, int chunkY, int chunkZ, size_t size){
    auto start = std::chrono::high_resolution_clock::now();

    /*
        MAKE SURE THAT ALL THE MASKS EXIST, CRASHES OTHERWISE!
    */

    float jump = static_cast<float>(CHUNK_SIZE) / static_cast<float>(size);

    for(int x = 0;x < size;x++) for(int y = 0;y < size;y++) for(int z = 0;z < size;z++){
        float rx = static_cast<float>(x) * jump + chunkX * CHUNK_SIZE;
        float ry = static_cast<float>(y) * jump + chunkY * CHUNK_SIZE;
        float rz = static_cast<float>(z) * jump + chunkZ * CHUNK_SIZE;
        
        float value = transcribeNoiseValue(noise.GetNoise(rx, ry, rz),  ry);
        bool top = transcribeNoiseValue(noise.GetNoise(rx, ry + jump, rz), ry + jump) <= 0.5;

        if(value > 0.5){
            if(top){
                chunk->setBlock({x,y,z}, {BlockType::Grass});
            }
            else{
                chunk->setBlock({x,y,z}, {BlockType::Dirt});
            }
            //chunk.setBlock(x,y,z, {top ? BlockType::Grass : BlockType::Stone});

            //if(top && rand() % 30 == 0) generateOakTree(chunk,x,y+1,z);
        }
    }

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
void WorldGenerator::generateTerrainChunkAccelerated(Chunk* chunk, glm::ivec3 chunkPosition){
    auto start = std::chrono::high_resolution_clock::now();

    computeProgram.use();

    glm::vec3 worldPosition = chunkPosition * CHUNK_SIZE;
    glUniform3fv(worldPositionUniformID, 1, glm::value_ptr(worldPosition));

    //computeProgram.updateUniforms();
    //CHECK_GL_ERROR();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, computeBuffer->getID());


    //CHECK_GL_ERROR();
    
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

    //CHECK_GL_ERROR();

    if(first){
        for(int i = 0;i < 10;i++){
            std::cout << std::bitset<64>(reinterpret_cast<uint64_t*>(computeBuffer->data())[i]) << std::endl;
        }
        first = false;
    }
    if(!chunk->hasLayerOfType(BlockType::Grass)) chunk->createLayer(BlockType::Grass, {});

    uint64_t* data = reinterpret_cast<uint64_t*>(computeBuffer->data());
    chunk->getLayer(BlockType::Grass).field.setData(data);
    chunk->getSolidField().setData(data);
    
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Generated chunk (" << chunkPosition.x << "," << chunkPosition.y << "," << chunkPosition.z << ") in: " << duration << " microseconds" << std::endl;
}