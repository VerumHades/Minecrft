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

#include <game/entity.hpp>
#include <game/world_saving.hpp>
#include <game/chunk.hpp>
#include <worldgen/worldgen.hpp>
#include <vec_hash.hpp>

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
        std::unordered_map<glm::vec3, std::unique_ptr<Chunk>, Vec3Hash, Vec3Equal> chunks;
        std::vector<Entity> entities;

        void saveChunk(std::ofstream &file, Chunk& chunk);

    public:
        Block* getBlock(int x, int y, int z);
        bool setBlock(int x, int y, int z, Block index);

        void generateChunk(int x, int y, int z);
        void generateChunkMesh(int x, int y, int z, MultiChunkBuffer& buffer);
        
        Chunk* getChunk(int x, int y, int z);
        Chunk* getChunkFromBlockPosition(int x, int y, int z);
        glm::vec3 getGetChunkRelativeBlockPosition(int x, int y, int z);

        CollisionCheckResult checkForPointCollision(float x, float y, float z, bool includeRectangularColliderLess);
        CollisionCheckResult checkForRectangularCollision(float x, float y, float z, RectangularCollider* collider);

        RaycastResult raycast(float fromX, float fromY, float fromZ, float dirX, float dirY, float dirZ, float maxDistance);

        void drawEntities(ModelManager& manager, Camera& camera, bool depthMode  = false);
        void updateEntities();

        std::vector<Entity>& getEntities() {return entities;}

        void save(std::string filepath);
        void load(std::string filepath);
        void loadChunk(ByteArray& source);
};

ByteArray serializeChunk(Chunk& chunk);

extern size_t predefinedBlocksTotal;
#endif