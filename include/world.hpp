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


struct Vec2Hash {
    std::size_t operator()(const glm::vec2& v) const noexcept;
};

struct Vec2Equal {
    bool operator()(const glm::vec2& lhs, const glm::vec2& rhs) const noexcept;
};


typedef struct RaycastResult{
    std::optional<std::reference_wrapper<const Block>> hitBlock;
    bool hit;
    int x;
    int y;
    int z;

    float lastX;
    float lastY;
    float lastZ;
} RaycastResult;

typedef struct CollisionCheckResult{
    std::optional<std::reference_wrapper<const Block>> collidedBlock;
    bool collision;
    int x;
    int y;
    int z;
} CollisionCheckResult;

class World{
    private:
        std::unordered_map<glm::vec2, Chunk, Vec2Hash, Vec2Equal> chunks;

    public:
        std::optional<std::reference_wrapper<const Block>> getBlock(int x, int y, int z);
        int setBlock(int x, int y, int z, Block index);

        Chunk& generateChunk(int x, int y);
        std::optional<std::reference_wrapper<Chunk>> getChunk(int x, int z);
        std::optional<std::reference_wrapper<Chunk>> getChunkWithMesh(int x, int z);
        std::optional<std::reference_wrapper<Chunk>> getChunkFromBlockPosition(int x, int z);

        CollisionCheckResult checkForPointCollision(float x, float y, float z, int includeRectangularColliderLess);
        CollisionCheckResult checkForRectangularCollision(float x, float y, float z, RectangularCollider* collider);

        RaycastResult raycast(float fromX, float fromY, float fromZ, float dirX, float dirY, float dirZ, float maxDistance);
};

extern size_t predefinedBlocksTotal;
#endif