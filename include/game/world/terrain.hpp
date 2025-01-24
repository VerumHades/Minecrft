#ifndef WORLD_H
#define WORLD_H

#include <thread>
#include <glm/glm.hpp>
#include <unordered_map>
#include <optional>
#include <functional>
#include <mutex> 
#include <chrono>
#include <queue>
#include <atomic>

#include <iostream>
#include <fstream>
#include <string>

#include <rendering/chunk_buffer.hpp>

#include <game/entity.hpp>
#include <game/chunk.hpp>
#include <game/world/world_generation.hpp>
#include <vec_hash.hpp>
#include <game/threadpool.hpp>

#include <game/world/world_stream.hpp>

struct RaycastResult{
    glm::ivec3 position; // Position of the hit block
    glm::vec3 lastPosition; // Position before the hit
};

class Terrain{
    private:
        std::mutex mutex;
        std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, IVec3Hash, IVec3Equal> chunks;
        
    public:
        Terrain(){}
        Block* getBlock(glm::ivec3 position) const;
        bool setBlock(const glm::ivec3& position, const Block& index);

        Chunk* createEmptyChunk(glm::ivec3 position);
        void removeChunk(const glm::ivec3& position);

        Chunk* getChunk(glm::ivec3 position) const;
        Chunk* getChunkFromBlockPosition(glm::ivec3 position) const;
        
        glm::ivec3 blockToChunkPosition(glm::ivec3 blockPosition) const;
        glm::ivec3 getGetChunkRelativeBlockPosition(glm::ivec3 position);

        bool collision(glm::vec3 position, const RectangularCollider* collider);

        RaycastResult raycast(const glm::vec3& from, const glm::vec3& direction, float maxDistance);

        int chunksTotal() const {return chunks.size();}
};
#endif