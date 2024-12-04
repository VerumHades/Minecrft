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

class ExternalModelManager;

class World: public virtual Collidable{
    private:
        std::mutex mutex;
        std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, IVec3Hash, IVec3Equal> chunks;

        const int entity_region_size = 16;
        std::unordered_map<glm::ivec3, std::vector<std::shared_ptr<Entity>>, IVec3Hash, IVec3Equal> entity_regions;

        std::unique_ptr<WorldStream> stream;
        WorldGenerator generator;
        BlockRegistry&  blockRegistry;

        glm::ivec3 blockToChunkPosition(glm::ivec3 blockPosition) const;
        glm::ivec3 getEntityRegionPosition(const std::shared_ptr<Entity>& entity);

        void addEntityToRegion(const glm::ivec3 region_position, std::shared_ptr<Entity> entity);
        bool removeEntityFromRegion(const glm::ivec3 region_position, std::shared_ptr<Entity> entity);

        void drawEntity(const std::shared_ptr<Entity>& entity);
        void updateEntity(std::shared_ptr<Entity> entity);
        
    public:
        World(std::string filepath, BlockRegistry& blockRegistry);

        Block* getBlock(glm::ivec3 position) const;
        bool setBlock(glm::ivec3 position, Block index);

        Chunk* generateChunk(glm::ivec3 position);
        
        bool isChunkLoadable(glm::ivec3 position);
        void loadChunk(glm::ivec3 position);

        Chunk* getChunk(glm::ivec3 position) const;
        Chunk* getChunkFromBlockPosition(glm::ivec3 position) const;
        glm::ivec3 getGetChunkRelativeBlockPosition(glm::ivec3 position);

        std::tuple<bool, Block*> checkForPointCollision(glm::vec3 position, bool includeRectangularColliderLess);
        bool collidesWith(glm::vec3 position, RectangularCollider* collider) const override;

        RaycastResult raycast(glm::vec3 from, glm::vec3 direction, float maxDistance);

        void drawEntities();
        void updateEntities();

        void addEntity(std::shared_ptr<Entity> entity) {
            if(!entity) throw std::logic_error("Cannot add nullptr entity to a world.");
            updateEntity(entity); 
        }
        void drawEntityColliders(WireframeCubeRenderer& renderer, size_t start_index = 50);
        std::vector<std::shared_ptr<Entity>>& getRegionEntities(const glm::ivec3 region_position) override {
            return entity_regions[region_position];
        }

        int chunksTotal() const {return chunks.size();}
};
#endif