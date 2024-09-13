#ifndef WORLD_H
#define WORLD_H

#include <chunk.hpp>
#include <worldgen/worldgen.hpp>
#include <thread>
#include <glm/glm.hpp>
#include <unordered_map>
#include <optional>
#include <functional>
#include <shared_mutex> 
#include <chrono>
#include <queue>

#include <entity.hpp>

struct Vec3Hash {
    std::size_t operator()(const glm::vec3& v) const noexcept;
};

struct Vec3Equal {
    bool operator()(const glm::vec3& lhs, const glm::vec3& rhs) const noexcept;
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


class ModelManager;

class World: public Collidable{
    private:
        std::unordered_map<glm::vec3, Chunk, Vec3Hash, Vec3Equal> chunks;
        std::vector<Entity> entities;
        std::queue<glm::vec3> bufferLoadQue;

    public:
        bool updated = false;

        Block* getBlock(int x, int y, int z, int LOD);
        bool setBlock(int x, int y, int z, Block index);

        Chunk* generateAndGetChunk(int x, int y, int z);
        Chunk* getChunk(int x, int y, int z);
        Chunk* getChunkWithMesh(int x, int y, int z, int LOD);
        Chunk* getChunkFromBlockPosition(int x, int y, int z);
        glm::vec3 getGetChunkRelativeBlockPosition(int x, int y, int z);

        CollisionCheckResult checkForPointCollision(float x, float y, float z, bool includeRectangularColliderLess);
        CollisionCheckResult checkForRectangularCollision(float x, float y, float z, RectangularCollider* collider);

        RaycastResult raycast(float fromX, float fromY, float fromZ, float dirX, float dirY, float dirZ, float maxDistance);

        void drawChunks(Camera& camera, ShaderProgram& program, int renderDistance);
        void drawEntities(ModelManager& manager, Camera& camera, bool depthMode  = false);
        void updateBuffers();
        void updateEntities();

        std::vector<Entity>& getEntities() {return entities;}
};

extern size_t predefinedBlocksTotal;
#endif