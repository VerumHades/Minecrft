#include <game/world/world_generation.hpp>

#define FNL_IMPL
#include <FastNoiseLite.h>

/*float lerp(float a, float b, float f)
{
    return a * (1.0f - f) + (b * f);
}*/

std::random_device dev;
std::mt19937 rng(dev());
std::uniform_int_distribution<std::mt19937::result_type> dist6(1,10); // distribution in range [1, 6]

WorldGenerator::WorldGenerator(BlockRegistry&  blockRegistry):  blockRegistry(blockRegistry){
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
    noise.SetSeed(1984);
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

    BlockID grass_id = blockRegistry.getIndexByName("grass");
    BlockID dirt_id  = blockRegistry.getIndexByName("dirt");

    float jump = static_cast<float>(CHUNK_SIZE) / static_cast<float>(size);

    for(int x = 0;x < size;x++) for(int y = 0;y < size;y++) for(int z = 0;z < size;z++){
        float rx = static_cast<float>(x) * jump + chunkX * CHUNK_SIZE;
        float ry = static_cast<float>(y) * jump + chunkY * CHUNK_SIZE;
        float rz = static_cast<float>(z) * jump + chunkZ * CHUNK_SIZE;
        
        float value = transcribeNoiseValue(noise.GetNoise(rx, ry, rz),  ry);
        bool top = transcribeNoiseValue(noise.GetNoise(rx, ry + jump, rz), ry + jump) <= 0.5;

        if(value > 0.5){
            if(top){
                chunk->setBlock({x,y,z}, {grass_id});
            }
            else{
                chunk->setBlock({x,y,z}, {dirt_id});
            }
            //chunk.setBlock(x,y,z, {top ? BlockID::Grass : BlockID::Stone});

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
        if(block->type == BlockID::Stone && upperBlock && upperBlock->type == BLOCK_AIR_INDEX){
            chunk.setBlock(x,y,z, {BlockID::Grass});

            if(dist6(rng) == 100) generateOakTree(chunk, x,y,z);
        }
    }*/



      // End time point
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //std::cout << "Generated chunk (" << chunkX << "," << chunkY << "," << chunkZ << ") in: " << duration << " microseconds" << std::endl;

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
    glDispatchCompute(64, 64, 64 / 32);

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

    BlockID grass_id = blockRegistry.getIndexByName("grass");

    if(!chunk->hasLayerOfType(grass_id)) chunk->createLayer(grass_id, {});

    uint64_t* data = reinterpret_cast<uint64_t*>(computeBuffer->data());
    chunk->getLayer(grass_id).field.setData(data);
    chunk->getSolidField().setData(data);
    
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //std::cout << "Generated chunk (" << chunkPosition.x << "," << chunkPosition.y << "," << chunkPosition.z << ") in: " << duration << " microseconds" << std::endl;
}