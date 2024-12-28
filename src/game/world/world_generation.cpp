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
    for(auto& layer: compute_layers){
        layer.worldPositionUniformID = glGetUniformLocation(layer.program.getID(),"worldPosition");
        if(layer.worldPositionUniformID == -1) {
            std::cerr << "Generation program missing world position uniform for layer: " << layer.name << std::endl;
        }
        layer.program.setSamplerSlot("noiseTexture", 0);
    }

    computeBuffer.initialize(64 * 64  * (64 / 32));

    //CHECK_GL_ERROR();

    //noise = std::make_unique<FastNoiseLite>();
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.001f);
    noise.SetFractalOctaves(3);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(1984);

    const int noise_width = 1024;
    const int noise_height = 1024;

    Image noise_img{noise_width,noise_height,1};

    for(int x = 0;x < noise_width;x++){
        for(int y = 0;y < noise_height;y++){
            *noise_img.getPixel(x,y) = (noise.GetNoise(static_cast<float>(x),static_cast<float>(y)) + 1.0) / 2 * 80;
        }
    }

    noise_img.save("temp_noise.png");
    noiseTexture.configure(GL_R8, GL_RED, GL_UNSIGNED_BYTE, noise_width, noise_height, (void*) noise_img.getData(), 1);
    noiseTexture.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    noiseTexture.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    noiseTexture.parameter(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    noiseTexture.parameter(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);  
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
    glm::vec3 worldPosition = chunkPosition * CHUNK_SIZE;
    computeBuffer.bindBase(0);
    noiseTexture.bind(0);

    for(auto& layer: compute_layers){
        layer.program.use();

        glUniform3fv(layer.worldPositionUniformID, 1, glm::value_ptr(worldPosition));
        /*
            64 / 32 (division because one uint is 32 bits) by 64 by 64
        */
        glDispatchCompute(64, 64, 64 / 32);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        BlockID block_id = blockRegistry.getIndexByName(layer.name);
        if(!chunk->hasLayerOfType(block_id)) chunk->createLayer(block_id, {});

        computeBuffer.get(reinterpret_cast<uint*>(chunk->getLayer(block_id).field.data().data()), computeBuffer.size());

        chunk->getSolidField().applyOR(chunk->getLayer(block_id).field.data());
    }
}