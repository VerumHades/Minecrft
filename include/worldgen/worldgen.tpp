#include <worldgen/worldgen.hpp>

static inline float transcribeNoiseValue(float value, float ry){
    value -= std::max(ry / 256, -100.0f);
    value = std::max(0.0f, value);

    return value;
}

template <int size>
static inline void set(int x, int y, int z, ChunkMask<size>& solid, ChunkMask<size>& target){
    if(x < 0 || x > size || y < 0 || y > size || z < 0 || z > size) return;

    target.set(x,y,z);
    solid.set(x,y,z);
}

template <int size>
void WorldGenerator::generateTerrainChunk(Chunk& chunk, int chunkX, int chunkY, int chunkZ){
    //auto start = std::chrono::high_resolution_clock::now();

    auto group = std::make_unique<ChunkMaskGroup<size>>();
    group->masks[BlockTypes::Grass] = {BlockTypes::Grass};
    group->masks[BlockTypes::Stone] = {BlockTypes::Stone};
    group->masks[BlockTypes::OakLog] = {BlockTypes::OakLog};
    group->masks[BlockTypes::LeafBlock] = {BlockTypes::LeafBlock};
    group->masks[BlockTypes::GrassBillboard] = {BlockTypes::GrassBillboard};

    auto& grassMask = group->masks[BlockTypes::Grass];
    auto& stoneMask = group->masks[BlockTypes::Stone];
    auto& grassBillboardMask = group->masks[BlockTypes::GrassBillboard];

    auto& oaklogMask = group->masks[BlockTypes::OakLog];
    auto& leafMask = group->masks[BlockTypes::LeafBlock];

    int jump = CHUNK_SIZE / size;

    for(int x = 0;x < size;x++) for(int y = 0;y < size;y++) for(int z = 0;z < size;z++){
        float rx = (float)(x * jump + chunkX * CHUNK_SIZE);
        float ry = (float)(y * jump + chunkY * CHUNK_SIZE);
        float rz = (float)(z * jump + chunkZ * CHUNK_SIZE);
        
        float value = transcribeNoiseValue(noise.GetNoise(rx, ry, rz),  ry);
        bool top = transcribeNoiseValue(noise.GetNoise(rx, ry + jump, rz), ry + jump) <= 0.5;

        if(value > 0.5){
            if(top){
                set(x,y,z,group->solidMask,grassMask);

                if(top && rand() % 30 == 0) grassBillboardMask.set(x,y + 1,z);
                else if(top && rand() % 60 == 0) {
                    set(x,y + 1,z,group->solidMask,oaklogMask);
                    set(x,y + 2,z,group->solidMask,oaklogMask);
                    set(x,y + 3,z,group->solidMask,oaklogMask);
                    set(x,y + 4,z,group->solidMask,oaklogMask);
                } 
            }
            else{
                set(x,y,z,group->solidMask,stoneMask);
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