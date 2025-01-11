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

        std::unique_ptr<WorldStream> stream;
        WorldGenerator generator;

        glm::ivec3 blockToChunkPosition(glm::ivec3 blockPosition) const;

        void drawEntity(Entity& entity);

        std::atomic<bool> blocks_altered = false;
        
    public:
        World(std::string filepath);

        Block* getBlock(glm::ivec3 position) const;
        bool setBlock(const glm::ivec3& position, const Block& index);

        Chunk* generateChunk(glm::ivec3 position);
        Chunk* createEmptyChunk(glm::ivec3 position);
        
        bool isChunkLoadable(glm::ivec3 position);
        void loadChunk(glm::ivec3 position);

        Chunk* getChunk(glm::ivec3 position) const;
        Chunk* getChunkFromBlockPosition(glm::ivec3 position) const;
        glm::ivec3 getGetChunkRelativeBlockPosition(glm::ivec3 position);

        /*
            Saves all unsaved changes to chunks
        */
        void save();

        std::tuple<bool, Block*> checkForPointCollision(glm::vec3 position, bool includeRectangularColliderLess);
        bool collidesWith(glm::vec3 position, Entity* collider, bool vertical_check = false) override;

        RaycastResult raycast(glm::vec3 from, glm::vec3 direction, float maxDistance);

        void updateEntities(float deltatime);

        void addEntity(Entity entity) {
            entities.push_back(entity);
        }
        void drawEntityColliders(WireframeCubeRenderer& renderer, size_t start_index = 50);

        Entity& getPlayer(){return entities[0];}

        int chunksTotal() const {return chunks.size();}

        WorldGenerator& getWorldGenerator() {return generator;}
};
#endif