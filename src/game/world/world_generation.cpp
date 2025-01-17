#include <game/world/world_generation.hpp>

#define FNL_IMPL
#include <FastNoiseLite.h>

AcceleratedChunkGenerator::AcceleratedChunkGenerator(FastNoiseLite& noise): noise(noise) {
    computeBuffer.initialize(64 * 64  * (64 / 32));

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
}

void WorldGenerator::generateTerrainChunk(Chunk* chunk, glm::ivec3 position){
    generateTerrainChunkAccelerated(chunk, position);
}   

void WorldGenerator::generateTerrainChunkAccelerated(Chunk* chunk, glm::ivec3 chunkPosition){
    accelerated_generator->generateChunk(chunk, chunkPosition);
}