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
    ChunkDefinition({CT::WALL,CT::WALL,CT::OPEN,CT::OPEN,CT::OPEN,CT::OPEN}),
    ChunkDefinition({CT::OPEN,CT::WALL,CT::OPEN,CT::OPEN,CT::OPEN,CT::OPEN}),
    ChunkDefinition({CT::OPEN,CT::OPEN,CT::WALL,CT::WALL,CT::WALL,CT::WALL}),
    ChunkDefinition({CT::WALL,CT::OPEN,CT::OPEN,CT::OPEN,CT::OPEN,CT::OPEN}),
});

static std::unordered_map<glm::ivec3, size_t, IVec3Hash, IVec3Equal> generated_chunks{
    {glm::ivec3{0,0,0}, 1}
};

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

    for(int i = 0; i < 6;i++){
        glm::ivec3 offset_position = position + offsets[i];
        if(!generated_chunks.contains(offset_position)){
            surrounding_neighbours[i] = UNKNOWN;
            continue;
        }
        auto neighbour = chunk_definitions[generated_chunks.at(offset_position)];

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

        candidates.push_back(i);
    }

    if(candidates.size() == 0) candidates.push_back(0);

    // Define a fixed seed for reproducibility
    std::seed_seq seed{iseed};  // You can change this seed
    std::mt19937 gen(seed); // Mersenne Twister engine with fixed seed
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
    if(neighbours[TOP]    == WALL) placePlane(chunk, offset, size, size, Y);
    if(neighbours[BOTTOM] == WALL) placePlane(chunk, offset + glm::ivec3{0,15,0}, size, size, Y);

    if(neighbours[LEFT]   == WALL) placePlane(chunk, offset, size, size, X);
    if(neighbours[RIGHT]  == WALL) placePlane(chunk, offset + glm::ivec3{15,0,0}, size, size, X);

    if(neighbours[FRONT]  == WALL) placePlane(chunk, offset, size, size, Z);
    if(neighbours[BACK]   == WALL) placePlane(chunk, offset + glm::ivec3{0,0,15}, size, size, Z);
}

void WorldGenerator::generateTerrainChunk(Chunk* chunk, glm::ivec3 position){
    static const int count = CHUNK_SIZE / ChunkDefinition::size;

    for(int x = 0; x <= count; x++) 
    for(int y = 0; y <= count; y++) 
    for(int z = 0; z <= count; z++){
        glm::ivec3 localPosition = glm::ivec3(x,y,z);
        glm::ivec3 chunkPosition = (position * count) + localPosition;

        ChunkDefinition::getDefinitionFor(chunkPosition, seed).place(chunk, localPosition * ChunkDefinition::size);
    }
}   