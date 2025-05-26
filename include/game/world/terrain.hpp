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

#include <rendering/region_culler.hpp>

#include <game/entity.hpp>
#include <game/chunk.hpp>
#include <game/world/world_generation.hpp>
#include <vec_hash.hpp>
#include <game/threadpool.hpp>

#include <game/world/world_stream.hpp>

class GameState;

struct RaycastResult{
    glm::ivec3 position; // Position of the hit block
    glm::vec3 lastPosition; // Position before the hit
};

/**
 * @brief A class that holds chunks of block data to represent an 'infinite' world
 * 
 */
class Terrain{
    private:
        std::mutex mutex;
        std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, IVec3Hash, IVec3Equal> chunks{};

    public:
        Terrain(){}
        Block* getBlock(glm::ivec3 position) const;
        bool setBlock(const glm::ivec3& position, const Block& index);

        Chunk* createEmptyChunk(glm::ivec3 position);

        void addChunk(const glm::ivec3& position, std::unique_ptr<Chunk> chunk);

        /**
         * @brief Pull a chunk out of the world
         * 
         * @param position 
         * @return std::unique_ptr<Chunk> 
         */
        std::unique_ptr<Chunk> takeChunk(const glm::ivec3& position);

        /**
         * @brief Deletes a chunk completely
         * 
         * @param position 
         */
        void removeChunk(const glm::ivec3& position);

        Chunk* getChunk(glm::ivec3 position) const;

        /**
         * @brief Get chunk based on a world position of a block
         * 
         * @param position 
         * @return Chunk* 
         */
        Chunk* getChunkFromBlockPosition(glm::ivec3 position) const;

        /**
         * @brief Returns the position of the chunk based on a world position, world position => chunk position
         * 
         * @param blockPosition 
         * @return glm::ivec3 
         */
        glm::ivec3 blockToChunkPosition(glm::ivec3 blockPosition) const;

        /**
         * @brief Get the relative position to the corresponding chunk for a world block position, world position => blocks position inside the chunk its in
         * 
         * @param blockPosition 
         * @return glm::ivec3 
         */
        glm::ivec3 getGetChunkRelativeBlockPosition(glm::ivec3 position);

        /**
         * @brief Check for collision with the world for a rectangular collider
         * 
         * @param position 
         * @param collider 
         * @return true 
         * @return false 
         */
        bool collision(glm::vec3 position, const RectangularCollider* collider);

        /**
         * @brief Cast a ray trough the world and return the first intersection or no intersection if max distance was reached
         * 
         * @param from start position
         * @param direction a normalized direction
         * @param maxDistance 
         * @return RaycastResult 
         */
        RaycastResult raycast(const glm::vec3& from, const glm::vec3& direction, float maxDistance);

        int chunksTotal() const {return chunks.size();}

        friend class GameState;
};
#endif
