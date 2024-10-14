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
#include <game/threadpool.hpp>

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


class WorldStream{
    private:
        std::fstream file_stream;
        std::unordered_map<glm::vec3, size_t, Vec3Hash, Vec3Equal> chunkTable = {}; // Chunk locations in the file
        std::shared_mutex mutex;

        struct Header{
            char name[256];
            int seed;

            size_t chunk_table_start;
            size_t chunk_table_size;

            size_t chunk_data_start;
            size_t chunk_data_end;
        };

        Header header;

        ByteArray serializeTableData();
        void saveHeader();
        void loadHeader();

        void loadTable();
        void saveTable();

        size_t moveChunk(size_t from, size_t to);

    public:
        WorldStream(std::string filepath);
        ~WorldStream();
        void save(Chunk& chunk);
        void load(Chunk* chunk);
        bool hasChunkAt(glm::vec3 position);
        int getSeed() {return header.seed;};
};

class ModelManager;

class World: public Collidable{
    private:
        std::unordered_map<glm::vec3, std::unique_ptr<Chunk>, Vec3Hash, Vec3Equal> chunks;
        std::vector<Entity> entities;

        std::unique_ptr<WorldStream> stream;
        WorldGenerator generator;

    public:
        World(std::string filepath);
        Block* getBlock(int x, int y, int z);
        bool setBlock(int x, int y, int z, Block index);

        void generateChunk(int x, int y, int z);
        void generateChunkMesh(int x, int y, int z, MultiChunkBuffer& buffer, ThreadPool& pool);

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

        std::vector<Entity>& getEntities() {return entities;}
};
#endif