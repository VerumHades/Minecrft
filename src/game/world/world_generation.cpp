#include <game/world/world_generation.hpp>

#define FNL_IMPL
#include <FastNoiseLite.h>

using namespace WorldGeneration;
using CT = ChunkDefinition::CrossType;

WorldGenerator::WorldGenerator() {
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.001f);
    noise.SetFractalOctaves(3);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(1984);

}

static auto chunk_definitions = std::to_array({
    ChunkDefinition({CT::OPEN,CT::OPEN,CT::OPEN,CT::OPEN,CT::OPEN,CT::OPEN}),

    ChunkDefinition({CT::WALL,CT::WALL,CT::WALL,CT::WALL,CT::OPEN,CT::OPEN}),
    ChunkDefinition({CT::WALL,CT::WALL,CT::OPEN,CT::OPEN,CT::WALL,CT::WALL}),

    ChunkDefinition({CT::WALL,CT::WALL,CT::WALL,CT::OPEN,CT::OPEN,CT::WALL}),
    ChunkDefinition({CT::WALL,CT::WALL,CT::WALL,CT::OPEN,CT::WALL,CT::OPEN}),
    ChunkDefinition({CT::WALL,CT::WALL,CT::OPEN,CT::WALL,CT::OPEN,CT::WALL}),
    ChunkDefinition({CT::WALL,CT::WALL,CT::OPEN,CT::WALL,CT::WALL,CT::OPEN}),


    ChunkDefinition({CT::OPEN,CT::OPEN,CT::WALL,CT::WALL,CT::WALL,CT::WALL}),
    /*
        Open top
    */

    ChunkDefinition({CT::OPEN,CT::WALL,CT::WALL,CT::WALL,CT::OPEN,CT::OPEN}),
    ChunkDefinition({CT::OPEN,CT::WALL,CT::OPEN,CT::OPEN,CT::WALL,CT::WALL}),

    ChunkDefinition({CT::OPEN,CT::WALL,CT::WALL,CT::OPEN,CT::OPEN,CT::WALL}),
    ChunkDefinition({CT::OPEN,CT::WALL,CT::WALL,CT::OPEN,CT::WALL,CT::OPEN}),
    ChunkDefinition({CT::OPEN,CT::WALL,CT::OPEN,CT::WALL,CT::OPEN,CT::WALL}),
    ChunkDefinition({CT::OPEN,CT::WALL,CT::OPEN,CT::WALL,CT::WALL,CT::OPEN}),

    /*
        Open bottom
    */
    ChunkDefinition({CT::WALL,CT::OPEN,CT::WALL,CT::WALL,CT::OPEN,CT::OPEN}),
    ChunkDefinition({CT::WALL,CT::OPEN,CT::OPEN,CT::OPEN,CT::WALL,CT::WALL}),

    ChunkDefinition({CT::WALL,CT::OPEN,CT::WALL,CT::OPEN,CT::OPEN,CT::WALL}),
    ChunkDefinition({CT::WALL,CT::OPEN,CT::WALL,CT::OPEN,CT::WALL,CT::OPEN}),
    ChunkDefinition({CT::WALL,CT::OPEN,CT::OPEN,CT::WALL,CT::OPEN,CT::WALL}),
    ChunkDefinition({CT::WALL,CT::OPEN,CT::OPEN,CT::WALL,CT::WALL,CT::OPEN}),
});

static std::unordered_map<glm::ivec3, size_t, IVec3Hash, IVec3Equal> generated_chunks{};

const ChunkDefinition& ChunkDefinition::getDefinitionFor(const glm::ivec3& position, int iseed){
    if(generated_chunks.contains(position)) 
        return chunk_definitions.at(generated_chunks.at(position));

    const static std::array<glm::ivec3,6> offsets = {
        glm::ivec3{ 0, 1, 0},
        glm::ivec3{ 0,-1, 0},
        glm::ivec3{-1, 0, 0},
        glm::ivec3{ 1, 0, 0},
        glm::ivec3{ 0, 0,-1},
        glm::ivec3{ 0, 0, 1}
    };

    std::array<CT, 6> surrounding_neighbours{};
    std::unordered_set<size_t> found_neighbours{};

    for(int i = 0; i < 6;i++){
        glm::ivec3 offset_position = position + offsets[i];
        if(!generated_chunks.contains(offset_position)){
            surrounding_neighbours[i] = UNKNOWN;
            continue;
        }
        size_t neighbour_index = generated_chunks.at(offset_position);
        found_neighbours.emplace(neighbour_index);
        auto neighbour = chunk_definitions.at(neighbour_index);

        surrounding_neighbours[i] = neighbour.getCrossTypes().at(i + (i % 2 == 0 ? 1 : -1));
    }

    static std::vector<size_t> candidates{};
    candidates.clear();

    for(size_t i = 1;i < chunk_definitions.size();i++){
        auto definition = chunk_definitions.at(i);

        bool passed = true;
        for(int i = 0;i < 6;i++){
            if(surrounding_neighbours[i] == UNKNOWN) continue;
            if(definition.getCrossTypes().at(i) != surrounding_neighbours[i]){
                passed = false;
                break;
            }
        }

        if(!passed) continue;

        if(
            found_neighbours.contains(i)
            && chunk_definitions.at(i).getCrossTypes().at(BOTTOM) == WALL
            && chunk_definitions.at(i).getCrossTypes().at(TOP) == WALL
        ){
            for(int i = 0;i < 3;i++) candidates.push_back(i);
        }
        candidates.push_back(i);
    }

    if(candidates.size() == 0) candidates.push_back(0);

    // Define a fixed seed for reproducibility
    static std::seed_seq seed{iseed};  // You can change this seed
    static std::mt19937 gen(seed); // Mersenne Twister engine with fixed seed
    std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);

    size_t result = candidates.at(dist(gen));

    //std::cout << "Selected: " << result << " for " << position.x << " " << position.y << " " << position.z << "\n";  
    generated_chunks[position] = result;
    return chunk_definitions.at(result);
} 

enum Axis{
    X = 0,
    Y = 1,
    Z = 2
};

static void placePlane(Chunk* chunk, const glm::ivec3& origin, int width, int height, Axis axis){
    static const std::array<std::array<Axis, 2>,3> positionaires = {
        std::array<Axis, 2>{Y,Z},
        std::array<Axis, 2>{X,Z},
        std::array<Axis, 2>{X,Y}
    };

    auto indices = positionaires[axis];

    auto stone = BlockRegistry::get().getIndexByName("stone");

    glm::ivec3 position = origin;
    for(int horizontal = 0;horizontal < width; horizontal++)
    for(int vertical   = 0;vertical  < height; vertical++)
    {
        position[indices[0]] = origin[indices[0]] + horizontal;
        position[indices[1]] = origin[indices[1]] + vertical;
        
        chunk->setBlock(position, {stone}, true);
    }
}

void ChunkDefinition::place(Chunk* chunk, const glm::ivec3& offset) const {
    if(neighbours[TOP]    == WALL) placePlane(chunk, offset + glm::ivec3{0,size - 1,0}, size, size, Y);
    if(neighbours[BOTTOM] == WALL) placePlane(chunk, offset, size, size, Y);

    if(neighbours[LEFT]   == WALL) placePlane(chunk, offset, size, size, X);
    if(neighbours[RIGHT]  == WALL) placePlane(chunk, offset + glm::ivec3{size - 1,0,0}, size, size, X);

    if(neighbours[FRONT]  == WALL) placePlane(chunk, offset, size, size, Z);
    if(neighbours[BACK]   == WALL) placePlane(chunk, offset + glm::ivec3{0,0,size - 1}, size, size, Z);
}

WorldGenerator::Heightmap& WorldGenerator::getHeightmapFor(glm::ivec3 position_in){
    static std::shared_mutex mutex;
    auto position = glm::ivec3(position_in.x, 0, position_in.z);

    {
        std::shared_lock lock(mutex);
        if(getHeightMaps().contains(position)) 
            return getHeightMaps().at(position);
    }

    std::unique_lock lock(mutex);
    auto& map = getHeightMaps()[position];

    map.lowest = INT32_MAX;
    map.highest = INT32_MIN;

    for(int x = 0; x < CHUNK_SIZE; x++) 
    for(int z = 0; z < CHUNK_SIZE; z++){
        glm::ivec3 localPosition = glm::ivec3(x,0,z) + position * CHUNK_SIZE;

        int value = pow(noise.GetNoise(static_cast<float>(localPosition.x),static_cast<float>(localPosition.z)) * 10, 3);
        map.lowest = std::min(value, map.lowest);
        map.highest = std::max(value, map.highest);
        map.heights[x][z] = value;
    }

    return map;
}

void WorldGenerator::generateTerrainChunk(Chunk* chunk, glm::ivec3 position){
    //static const int count = CHUNK_SIZE / ChunkDefinition::size;
    auto& heightMap = getHeightmapFor(position);

    auto stone = BlockRegistry::get().getIndexByName("stone");
    auto grass = BlockRegistry::get().getIndexByName("grass");

    if(heightMap.lowest - 1 > position.y * CHUNK_SIZE + CHUNK_SIZE){
        chunk->fill({stone});
        return;
    }
    if(heightMap.highest < position.y * CHUNK_SIZE){
        return;
    }

    for(int x = 0; x <= CHUNK_SIZE; x++) 
    for(int y = 0; y <= CHUNK_SIZE; y++) 
    for(int z = 0; z <= CHUNK_SIZE; z++){
        glm::ivec3 localPosition = glm::ivec3(x,y,z) + position * CHUNK_SIZE;
        
        if(localPosition.y > heightMap.heights[x][z]) continue;

        if(localPosition.y == heightMap.heights[x][z] && heightMap.heights[x][z] < 120) 
            chunk->setBlock(glm::ivec3(x,y,z), {grass}, true);
        else 
            chunk->setBlock(glm::ivec3(x,y,z), {stone}, true);
    }
}   

std::thread WorldGenerator::threadedQueueGeneration(std::queue<Chunk*>& queue){
    static int id = 0;
    int myid = id++;

    std::thread thread = std::thread([this, &queue, myid] {
        int count = 50;
        while(!queue.empty()){
            auto chunk = queue.front();
            queue.pop();

            this->generateTerrainChunk(chunk, chunk->getWorldPosition());
            if(count <= 0){
                std::cout << "Amount left " << myid << ":" << queue.size() << "\n";
                count = 50;
            }
            else count--;
        }
    });

    return thread;
}