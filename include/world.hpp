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
#include <vec_hash.hpp>

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
        std::unordered_map<glm::vec3, std::unique_ptr<Chunk>, Vec3Hash, Vec3Equal> chunks;
        std::vector<Entity> entities;
        std::queue<glm::vec3> bufferLoadQue;

    public:
        bool updated = false;

        Block* getBlock(int x, int y, int z);
        bool setBlock(int x, int y, int z, Block index);

        void generateChunk(int x, int y, int z);
        Chunk* getChunk(int x, int y, int z);
        Chunk* getChunkWithMesh(int x, int y, int z);
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