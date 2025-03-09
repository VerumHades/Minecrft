#include <game/world/world_generation.hpp>

#define FNL_IMPL
#include <FastNoiseLite.h>

WorldGenerator::WorldGenerator() {
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.001f);
    noise.SetFractalOctaves(3);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    //noise.SetSeed(1985);

    tree = std::make_shared<Structure>(5,7,5);
    
    BlockID wood_id = BlockRegistry::get().getIndexByName("oak_log");
    BlockID leaf_id = BlockRegistry::get().getIndexByName("oak_leaves");

    for(int i = 0;i < 5;i++) tree->setBlock({2,i,2}, {wood_id});

    for(int i = 0; i < 5;i++)
    for(int j = 0; j < 5;j++)
    for(int g = 0; g < 2;g++){
        if(i == 2 && j == 2) continue;
        tree->setBlock({i,4 + g,j}, {leaf_id});
    }
    for(int i = 1; i < 4;i++)
    for(int j = 1; j < 4;j++){
        tree->setBlock({i,6,j}, {leaf_id});
    }
}

void WorldGenerator::placeStructure(const glm::ivec3& position, const std::shared_ptr<Structure>& structure){
    structures.add(position, structure->getSize(), structure);
}

Image WorldGenerator::createPreview(int width, int height, float step){
    Image out{width,height,4};

    int half_width = width / 2;
    int half_height = height / 2;

    for(int x = -half_width; x < half_width; x++) 
    for(int z = -half_height; z < half_height; z++){
        auto position = glm::vec3{x,0,z} * step;
        float normalized = getNoiseValueAt(position);
        int value = getHeightAt(position);

        glm::vec3 color = {60,150,60};

        if(value < 10) color = {60,60,150};
        else if(value >= 120) color = {130,130,130};

        auto* pixel = out.getPixel(x + half_width, z + half_height);
        pixel[0] = color.r * normalized;
        pixel[1] = color.g * normalized;
        pixel[2] = color.b * normalized;
        pixel[3] = 255;
    }

    return out;
}

float WorldGenerator::getNoiseValueAt(const glm::vec3 position){
    return (noise.GetNoise(static_cast<float>(position.x),static_cast<float>(position.z)) + 1.0) / 2.0;
}

int WorldGenerator::getHeightAt(const glm::vec3 position){
    return pow(getNoiseValueAt(position) * 14, 2);
}

WorldGenerator::Heightmap& WorldGenerator::getHeightmapFor(glm::ivec3 position_in){
    static std::shared_mutex mutex;
    auto position = glm::ivec3(position_in.x, 0, position_in.z);

    {
        std::shared_lock lock(mutex);
        if(getHeightMaps().contains(position)) 
            return *getHeightMaps().at(position);
    }

    std::unique_lock lock(mutex);
    getHeightMaps().emplace(position,std::make_unique<Heightmap>());
    auto& map = getHeightMaps().at(position);

    map->lowest = INT32_MAX;
    map->highest = INT32_MIN;

    for(int x = 0; x < CHUNK_SIZE; x++) 
    for(int z = 0; z < CHUNK_SIZE; z++){
        glm::ivec3 localPosition = glm::ivec3(x,0,z) + position * CHUNK_SIZE;

        int value = getHeightAt(localPosition);

        static std::seed_seq iseed{seed};  // You can change this seed
        static std::mt19937 gen(iseed); // Mersenne Twister engine with fixed seed
        static std::uniform_int_distribution<std::size_t> dist(0, 50);

        if(dist(gen) == 0 && localPosition.y + value > water_level) placeStructure(localPosition + glm::ivec3{-2,value,-2}, tree);
        
        map->lowest = std::min(value, map->lowest);
        map->highest = std::max(value, map->highest);
        map->heights[x][z] = value;
    }

    return *map;
}

void WorldGenerator::prepareHeightMaps(glm::ivec3 around, int distance){
    for(int i = -distance;i <= distance;i++)
    for(int j = -distance;j <= distance;j++)
    {
        getHeightmapFor(around + glm::ivec3{i, 0, j});
    }
}

bool WorldGenerator::isChunkSkipable(Chunk* chunk, const glm::ivec3 position){
    auto& heightMap = getHeightmapFor(position);
    auto stone = BlockRegistry::get().getIndexByName("stone");

    if(heightMap.lowest - 1 > position.y * CHUNK_SIZE + CHUNK_SIZE){
        chunk->fill({stone});
        return true;
    }
    if(heightMap.highest < position.y * CHUNK_SIZE){
        return true;
    }

    return false;
}

void WorldGenerator::generateTerrainChunk(Chunk* chunk, glm::ivec3 position){
    //static const int count = CHUNK_SIZE / ChunkDefinition::size;
    auto& heightMap = getHeightmapFor(position);
    
    if(heightMap.lowest - 1 > position.y * CHUNK_SIZE + CHUNK_SIZE){
        chunk->fill({stone});
        return;
    }
    if(heightMap.highest + CHUNK_SIZE < position.y * CHUNK_SIZE){
        return;
    }

    for(int x = 0; x < CHUNK_SIZE; x++) 
    for(int y = 0; y < CHUNK_SIZE; y++) 
    for(int z = 0; z < CHUNK_SIZE; z++){
        glm::ivec3 localPosition = glm::ivec3(x,y,z) + position * CHUNK_SIZE;

        auto* structure_region = structures.get(localPosition);
        if(structure_region && localPosition.y > water_level){
            glm::ivec3 relative_position = localPosition - structure_region->min;

            auto* block = structure_region->value->getBlock(relative_position);
            if(block && block->id != BLOCK_AIR_INDEX){
                chunk->setBlock(glm::ivec3(x,y,z), {block->id}, true);
                continue;
            }
        }

        if(localPosition.y > heightMap.heights[x][z]){
            if(localPosition.y <= water_level){
                chunk->setBlock(glm::ivec3(x,y,z), {blue_wool}, true);
                continue;
            }
            continue;
        }

        if(localPosition.y == heightMap.heights[x][z] && heightMap.heights[x][z] < 120) 
            chunk->setBlock(glm::ivec3(x,y,z), {grass}, true);
        else 
            chunk->setBlock(glm::ivec3(x,y,z), {stone}, true);
    }
}   

std::thread WorldGenerator::threadedQueueGeneration(std::queue<Chunk*>& queue, std::atomic<int>& progress_report, std::atomic<bool>& stop){
    static int id = 0;
    int myid = id++;

    std::thread thread = std::thread([this, &queue, myid, &progress_report, &stop] {
        int count = 50;
        while(!queue.empty()){
            if(stop) break;
            auto chunk = queue.front();
            queue.pop();

            this->generateTerrainChunk(chunk, chunk->getWorldPosition());
            if(count <= 0){
                progress_report.store(queue.size());
                count = 50;
            }
            else count--;
        }
    });


    return thread;
}