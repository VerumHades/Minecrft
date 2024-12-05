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

        std::vector<Entity> entities;

        const int entity_region_size = 4;
        std::unordered_map<glm::ivec3, std::vector<Entity*>, IVec3Hash, IVec3Equal> entity_regions;

        std::unique_ptr<WorldStream> stream;
        WorldGenerator generator;
        BlockRegistry&  blockRegistry;

        glm::ivec3 blockToChunkPosition(glm::ivec3 blockPosition) const;
        glm::ivec3 getEntityRegionPosition(const Entity& entity) const;
        glm::ivec3 getRegionPosition(glm::vec3 position) const;

        void addEntityToRegion(const glm::ivec3 region_position, Entity& entity);
        bool removeEntityFromRegion(const glm::ivec3 region_position, Entity& entity);

        void drawEntity(Entity& entity);
        void updateEntity(Entity& entity, int& index);
        
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
        bool collidesWith(glm::vec3 position, Entity* collider) const override;

        RaycastResult raycast(glm::vec3 from, glm::vec3 direction, float maxDistance);

        void drawEntities();
        void updateEntities();

        void addEntity(Entity entity) {
            entities.push_back(entity);
        }
        void drawEntityColliders(WireframeCubeRenderer& renderer, size_t start_index = 50);

        Entity& getPlayer(){return entities[0];}
        std::vector<Entity>& getEntities() {return entities;}

        int chunksTotal() const {return chunks.size();}
};
#endif