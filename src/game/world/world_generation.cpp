#include <game/world/world_generation.hpp>

#define FNL_IMPL
#include <FastNoiseLite.h>

AcceleratedChunkGenerator::AcceleratedChunkGenerator(FastNoiseLite& noise): noise(noise) {
    computeBuffer.initialize(64 * 64  * (64 / 32));

    const int noise_width = 1024;
    const int noise_height = 1024;

    Image noise_img{noise_width,noise_height,1};

    for(int x = 0;x < noise_width;x++){
        for(int y = 0;y < noise_height;y++){
            *noise_img.getPixel(x,y) = (noise.GetNoise(static_cast<float>(x),static_cast<float>(y)) + 1.0) / 2 * 80;
        }
    }

    noiseTexture.configure(GL_R8, GL_RED, GL_UNSIGNED_BYTE, noise_width, noise_height, (void*) noise_img.getData(), 1);
    noiseTexture.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    noiseTexture.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    noiseTexture.parameter(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    noiseTexture.parameter(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);  
}

ComputeLayer ComputeLayer::FromTemplate(std::string name, SourceTemplate& template_, std::vector<SourceTemplate::TagValue> values){
    std::string source = template_.fill(values);
    
    ComputeLayer layer;
    
    layer.program = std::make_shared<ShaderProgram>();
    layer.program->addShaderSource(source, GL_COMPUTE_SHADER);
    layer.program->compile();

    layer.name = name;

    layer.worldPositionUniformID = glGetUniformLocation(layer.program->getID(),"worldPosition");
    if(layer.worldPositionUniformID == -1) {
        std::cerr << "Generation program missing world position uniform for layer: " << layer.name << std::endl;
        return layer;
    }

    layer.program->setSamplerSlot("noiseTexture", 0);

    return layer;
}

void AcceleratedChunkGenerator::addComputeLayer(std::string condition, std::string name){
    compute_layers.push_back(ComputeLayer::FromTemplate(name, base_template, {{"condition", condition}}));
}

bool first = true;
void AcceleratedChunkGenerator::generateChunk(Chunk* chunk, glm::ivec3 chunkPosition){
    glm::vec3 worldPosition = chunkPosition * CHUNK_SIZE;
    computeBuffer.bindBase(0);
    noiseTexture.bind(0);

    for(auto& layer: compute_layers){
        layer.program->use();

        glUniform3fv(layer.worldPositionUniformID, 1, glm::value_ptr(worldPosition));
        /*
            64 / 32 (division because one uint is 32 bits) by 64 by 64
        */
        glDispatchCompute(64, 64, 64 / 32);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //timer.timestamp("GPU generated");
        std::array<uint64_t, 64 * 64> data = {};
        computeBuffer.get(reinterpret_cast<uint*>(data.data()),computeBuffer.size());
        //timer.timestamp("Fetched from GPU");
        bool empty = true;
        for(int i = 0;i < 64 * 64;i++){
            if(data[i] == 0ULL) continue;
            empty = false;
            break;
        }
        //timer.timestamp("Checked for emptiness");
        if(empty) continue;
        //timer.timestamp("Compressed and decompressed");

        //std::cout << "Compressed size: " << compressed.size() * 8 << "bytes" << std::endl;
        //std::cout << "Compression percent for: " << layer.name << " " << ((float) compressed.size() / (float) data.size()) * 100 << "%" << std::endl;

        BlockID block_id = global_block_registry.getIndexByName(layer.name);

        if(!chunk->hasLayerOfType(block_id)) chunk->createLayer(block_id, {});

        chunk->getLayer(block_id).field().data() = data;
        chunk->getSolidField().applyOR(data);
    }
}

RegionChunkGenerator::RegionChunkGenerator(FastNoiseLite& noise): noise(noise){
    const int noise_width = 1024;
    const int noise_height = 1024;

    Image noise_img{noise_width,noise_height,1};

    for(int x = 0;x < noise_width;x++){
        for(int y = 0;y < noise_height;y++){
            *noise_img.getPixel(x,y) = (noise.GetNoise(static_cast<float>(x),static_cast<float>(y)) + 1.0) / 2 * 80;
        }
    }

    noiseTexture.configure(GL_R8, GL_RED, GL_UNSIGNED_BYTE, noise_width, noise_height, (void*) noise_img.getData(), 1);
    noiseTexture.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    noiseTexture.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    noiseTexture.parameter(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    noiseTexture.parameter(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT); 

    base_profile = createProfile({4,4,32},glm::ivec3{64,64,64} * 4,{"int(position.y) == int(current_height)"});
}

GenerationProfile RegionChunkGenerator::createProfile(glm::ivec3 work_group_size, glm::ivec3 work_groups, std::vector<std::string> conditions){
    GenerationProfile profile{
        work_group_size,
        work_groups,
        {}
    };

    for(auto& condition: conditions){
        profile.compute_layers.push_back(
            ComputeLayer::FromTemplate(condition,base_region_template,
                {
                    {"condition", condition},
                    {"size_x", std::to_string(work_group_size.x)},
                    {"size_y", std::to_string(work_group_size.y)},
                    {"size_z", std::to_string(work_group_size.z)}
                }
            )
        );
    }

    return profile;
}

void RegionChunkGenerator::generateWithBaseProfile(World& world, glm::ivec3 region_start){
    generate(world,region_start,base_profile);
}

void RegionChunkGenerator::generate(World& world, glm::ivec3 region_start, GenerationProfile& profile){
    ScopeTimer timer("Region generated");

    if(profile.work_group_size.z % 32 != 0) {
        std::cerr << "Work  group size on z axis must be multiple of 32." << std::endl;
        return;
    }

    size_t z_size = profile.work_groups.z / 32;

    size_t work_group_size_total = profile.work_group_size.x * profile.work_group_size.y * z_size;
    size_t work_groups_total = profile.work_groups.x * profile.work_groups.y * profile.work_groups.z;

    size_t size_total = work_group_size_total * work_groups_total;

    if(computeBuffer.size() != size_total) computeBuffer.initialize(size_total);

    if((profile.work_group_size * profile.work_groups) % CHUNK_SIZE != glm::ivec3{0,0,0}){
        std::cerr << "Invalid sizes in generation profile." << std::endl;
        return;
    }

    glm::ivec3 chunk_counts = (profile.work_group_size * profile.work_groups) / CHUNK_SIZE;

    std::vector<Chunk*> chunks(chunk_counts.x * chunk_counts.y * chunk_counts.z); 

    /*
        Create all chunks
    */
    for(int x = 0; x < chunk_counts.x;x++)
    for(int y = 0; y < chunk_counts.y;y++)
    for(int z = 0; z < chunk_counts.z;z++)
    {
        glm::ivec3 position = region_start + glm::ivec3{x,y,z};

        Chunk* chunk = nullptr;
        if(!world.getChunk(position)) chunk = world.createEmptyChunk(position);

        chunks[x + y * chunk_counts.x + z * (chunk_counts.x * chunk_counts.y)] = chunk;
    }

    /*
        Actually fetch them because creating can invalidate pointers
    */
    for(int x = 0; x < chunk_counts.x;x++)
    for(int y = 0; y < chunk_counts.y;y++)
    for(int z = 0; z < chunk_counts.z;z++)
    {
        glm::ivec3 position = region_start + glm::ivec3{x,y,z};

        auto& chunk_ptr = chunks[x + y * chunk_counts.x + z * (chunk_counts.x * chunk_counts.y)];
        if(chunk_ptr) chunk_ptr = world.createEmptyChunk(position);
    }

    glm::vec3 worldPosition = region_start * CHUNK_SIZE;
    const size_t chunk_data_size = 64 * 64;
    computeBuffer.bindBase(0);
    noiseTexture.bind(0);

    for(auto& layer: profile.compute_layers){
        layer.program->use();

        glUniform3fv(layer.worldPositionUniformID, 1, glm::value_ptr(worldPosition));
        /*
            64 / 32 (division because one uint is 32 bits) by 64 by 64
        */
        glDispatchCompute(profile.work_groups.x, profile.work_groups.y, profile.work_groups.z);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //timer.timestamp("GPU generated");
        std::vector<uint64_t> all_generated_data(size_total / 2);
        computeBuffer.get(reinterpret_cast<uint*>(all_generated_data.data()),computeBuffer.size());

        BlockID block_id = global_block_registry.getIndexByName(layer.name);

        for(int x = 0; x < chunk_counts.x;x++)
        for(int y = 0; y < chunk_counts.y;y++)
        for(int z = 0; z < chunk_counts.z;z++)
        {
            size_t chunk_index = x + y * chunk_counts.x + z * (chunk_counts.x * chunk_counts.y);

            auto& chunk_ptr = chunks[chunk_index];

            if(!chunk_ptr) continue;

            uint64_t* chunk_data = all_generated_data.data() + (chunk_index * chunk_data_size);
            //timer.timestamp("Fetched from GPU");
            bool empty = true;
            for(int i = 0;i < 64 * 64;i++){
                if(chunk_data[i] == 0ULL) continue;
                empty = false;
                break;
            }

            if(empty) continue;

            if(!chunk_ptr->hasLayerOfType(block_id)) chunk_ptr->createLayer(block_id, {});

            auto& layer_data = chunk_ptr->getLayer(block_id).field().data();
            std::memcpy(layer_data.data(), chunk_data, layer_data.size() * sizeof(uint64_t));
            chunk_ptr->getSolidField().applyOR(layer_data);
        }
    }
}

WorldGenerator::WorldGenerator() {
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.001f);
    noise.SetFractalOctaves(3);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(1984);

    accelerated_generator = std::make_unique<AcceleratedChunkGenerator>(noise);
    accelerated_generator->addComputeLayer("int(position.y) == int(current_height)","grass");
    accelerated_generator->addComputeLayer("(position.y < current_height) && (position.y + 3 > current_height)", "dirt");
    accelerated_generator->addComputeLayer("(position.y <= current_height - 3)", "stone");

    region_generator = std::make_unique<RegionChunkGenerator>(noise);
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

    BlockID grass_id = global_block_registry.getIndexByName("grass");
    BlockID dirt_id  = global_block_registry.getIndexByName("dirt");

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

void WorldGenerator::generateTerrainChunkAccelerated(Chunk* chunk, glm::ivec3 chunkPosition){
    accelerated_generator->generateChunk(chunk, chunkPosition);
}

void WorldGenerator::generateChunkRegion(World& world, glm::ivec3 start){
    region_generator->generateWithBaseProfile(world, start);
}