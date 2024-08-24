#ifndef WORLD_H
#define WORLD_H

#include <chunk.hpp>
#include <ctime>
#include <worldgen/worldgen.hpp>
#include <thread>
#include <glm/glm.hpp>
#include <unordered_map>
#include <optional>
#include <functional>
#include <shared_mutex> 
#include <chrono>

struct Vec2Hash {
    std::size_t operator()(const glm::vec2& v) const noexcept;
};

struct Vec2Equal {
    bool operator()(const glm::vec2& lhs, const glm::vec2& rhs) const noexcept;
};


typedef struct RaycastResult{
    Block* hitBlock;
    bool hit;
    int x;
    int y;
    int z;

    float lastX;
    float lastY;
    float lastZ;
} RaycastResult;

typedef struct CollisionCheckResult{
    Block* collidedBlock;
    bool collision;
    int x;
    int y;
    int z;
} CollisionCheckResult;

class World{
    private:
        std::unordered_map<glm::vec2, Chunk, Vec2Hash, Vec2Equal> chunks;

    public:
        Block* getBlock(int x, int y, int z);
        bool setBlock(int x, int y, int z, Block index);

        Chunk* generateAndGetChunk(int x, int y);
        Chunk* getChunk(int x, int z);
        Chunk* getChunkWithMesh(int x, int z);
        Chunk* getChunkFromBlockPosition(int x, int z);

        CollisionCheckResult checkForPointCollision(float x, float y, float z, bool includeRectangularColliderLess);
        CollisionCheckResult checkForRectangularCollision(float x, float y, float z, RectangularCollider* collider);

        RaycastResult raycast(float fromX, float fromY, float fromZ, float dirX, float dirY, float dirZ, float maxDistance);
};

extern size_t predefinedBlocksTotal;
#endif