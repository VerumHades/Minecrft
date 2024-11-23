#ifndef WORLD_H
#define WORLD_H

#include <thread>
#include <glm/glm.hpp>
#include <unordered_map>
#include <optional>
#include <functional>
#include <shared_mutex> 
#include <chrono>
#include <queue>

#include <iostream>
#include <fstream>
#include <string>

#include <rendering/chunk_buffer.hpp>

#include <game/entity.hpp>
#include <game/world_saving.hpp>
#include <game/chunk.hpp>
#include <game/world/world_generation.hpp>
#include <vec_hash.hpp>
#include <game/threadpool.hpp>

#include <game/world/world_stream.hpp>

struct RaycastResult{
    Block* hitBlock;
    bool hit;
    glm::ivec3 position; // Position of the hit block
    glm::vec3 lastPosition; // Position before the hit
};

class ModelManager;

class World: public Collidable{
    private:
        std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, IVec3Hash, IVec3Equal> chunks;
        std::vector<Entity> entities;

        std::unique_ptr<WorldStream> stream;
        WorldGenerator generator;

        glm::ivec3 blockToChunkPosition(glm::ivec3 blockPosition);
        
    public:
        World(std::string filepath);
        Block* getBlock(glm::ivec3 position);
        bool setBlock(glm::ivec3 position, Block index);

        Chunk* generateChunk(glm::ivec3 position);
        
        bool isChunkLoadable(glm::ivec3 position);
        void loadChunk(glm::ivec3 position);

        Chunk* getChunk(glm::ivec3 position);
        Chunk* getChunkFromBlockPosition(glm::ivec3 position);
        glm::ivec3 getGetChunkRelativeBlockPosition(glm::ivec3 position);

        CollisionCheckResult checkForPointCollision(glm::vec3 position, bool includeRectangularColliderLess);
        CollisionCheckResult checkForRectangularCollision(glm::vec3 position, RectangularCollider* collider) override;

        RaycastResult raycast(glm::vec3 from, glm::vec3 direction, float maxDistance);

        void drawEntities(ModelManager& manager, Camera& camera, bool depthMode  = false);
        void updateEntities();

        int chunksTotal() const {return chunks.size();}
        std::vector<Entity>& getEntities() {return entities;}
};
#endif