#include <worldgen/worldgen.hpp>

static inline float transcribeNoiseValue(float value, float ry){
    value -= std::max(ry / 256, -100.0f);
    value = std::max(0.0f, value);

    return value;
}

template <int size>
void WorldGenerator::generateTerrainChunk(Chunk& chunk, int chunkX, int chunkY, int chunkZ){
    //auto start = std::chrono::high_resolution_clock::now();

    auto group = std::make_unique<ChunkMaskGroup<size>>();
    group->masks[BlockTypes::Grass] = {BlockTypes::Grass};
    group->masks[BlockTypes::Stone] = {BlockTypes::Stone};

    auto& grassMask = group->masks[BlockTypes::Grass];
    auto& stoneMask = group->masks[BlockTypes::Stone];

    int jump = CHUNK_SIZE / size;

    for(int x = 0;x < size;x++) for(int y = 0;y < size;y++) for(int z = 0;z < size;z++){
        float rx = (float)(x * jump + chunkX * CHUNK_SIZE);
        float ry = (float)(y * jump + chunkY * CHUNK_SIZE);
        float rz = (float)(z * jump + chunkZ * CHUNK_SIZE);
        
        float value = transcribeNoiseValue(noise.GetNoise(rx, ry, rz),  ry);
        bool top = transcribeNoiseValue(noise.GetNoise(rx, ry + 1, rz), ry + 1) <= 0.5;

        if(value > 0.5){
            if(top){
                grassMask.set(x,y,z);
                group->solidMask.set(x,y,z);
            }
            else{
                stoneMask.set(x,y,z);
                group->solidMask.set(x,y,z);
            }
            //chunk.setBlock(x,y,z, {top ? BlockTypes::Grass : BlockTypes::Stone});

            //if(top && rand() % 30 == 0) generateOakTree(chunk,x,y+1,z);
        }
    }

    chunk.setMainGroup(std::move(group));

    //std::cout << "Generated with mask:" << chunk.getMainGroupAs<64>() << " " << chunk.getMainGroupAs<64>()->masks.size() << std::endl;
    
    /*for(int x = 0;x < CHUNK_SIZE;x++) for(int y = 0;y < CHUNK_SIZE;y++) for(int z = 0;z < CHUNK_SIZE;z++){
        float rx = (float)(x + chunkX * CHUNK_SIZE);
        float ry = (float)(y + chunkY * CHUNK_SIZE);
        float rz = (float)(z + chunkZ * CHUNK_SIZE);

        Block* block = chunk.getBlock(x,y,z);
        Block* upperBlock = chunk.getBlock(x,y + 1,z); 
        if(block->type == BlockTypes::Stone && upperBlock && upperBlock->type == BlockTypes::Air){
            chunk.setBlock(x,y,z, {BlockTypes::Grass});

            if(dist6(rng) == 100) generateOakTree(chunk, x,y,z);
        }
    }*/



      // End time point
    //auto end = std::chrono::high_resolution_clock::now();

    //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //std::cout << "Generated chunk (" << chunkX << "," << chunkY << "," << chunkZ << ") in: " << duration << " microseconds" << std::endl;

}   