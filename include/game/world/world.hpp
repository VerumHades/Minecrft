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
    int x;
    int y;
    int z;

    float lastX;
    float lastY;
    float lastZ;
};

class ModelManager;

class World: public Collidable{
    private:
        std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, IVec3Hash, IVec3Equal> chunks;
        std::vector<Entity> entities;

        std::unique_ptr<WorldStream> stream;
        WorldGenerator generator;
        
    public:
        World(std::string filepath);
        Block* getBlock(int x, int y, int z);
        bool setBlock(int x, int y, int z, Block index);

        Chunk* generateChunk(int x, int y, int z, int lod);
        
        bool isChunkLoadable(int x, int y, int z);
        void loadChunk(int x, int y, int z);

        Chunk* getChunk(int x, int y, int z);
        Chunk* getChunkFromBlockPosition(int x, int y, int z);
        glm::vec3 getGetChunkRelativeBlockPosition(int x, int y, int z);

        CollisionCheckResult checkForPointCollision(float x, float y, float z, bool includeRectangularColliderLess);
        CollisionCheckResult checkForRectangularCollision(float x, float y, float z, RectangularCollider* collider);

        RaycastResult raycast(float fromX, float fromY, float fromZ, float dirX, float dirY, float dirZ, float maxDistance);

        void drawEntities(ModelManager& manager, Camera& camera, bool depthMode  = false);
        void updateEntities();

        int chunksTotal() const {return chunks.size();}
        std::vector<Entity>& getEntities() {return entities;}
};
#endif