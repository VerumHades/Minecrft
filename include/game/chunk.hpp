#ifndef CHUNK_H
#define CHUNK_H

#include <cmath>
#include <ctime>

#include <rendering/texture.hpp>
#include <rendering/buffer.hpp>
#include <rendering/chunk_buffer.hpp>
#include <rendering/mesh.hpp>
#include <rendering/camera.hpp>
#include <rendering/model.hpp>
#include <rendering/bitworks.hpp>
#include <game/threadpool.hpp>
#include <game/chunk_masks.hpp>

#include <glm/glm.hpp>
#include <map>
#include <optional>
#include <functional>
#include <mutex>
#include <memory>
#include <array>
#include <game/blocks.hpp>
#include <ranges>
#include <iterator>

#include <bit>
#include <bitset>
#include <iostream>

class World;

class Chunk: public Volume{
    private:
        glm::ivec3 worldPosition = glm::ivec3(0,0,0);
        World& world; 
        
        std::unique_ptr<DynamicChunkContents> currentGroup;
        
        void generateMeshes();

        bool generatedEmptyMesh = false;
        bool meshGenerating = false;
        
        std::unique_ptr<Mesh> solidMesh;
        
    public:
        //std::optional<Mesh> transparentMesh;

        Chunk(World& world, const glm::vec3& pos): world(world), worldPosition(pos){}

        bool isEmpty() {return currentGroup && currentGroup->empty();}
        bool isMeshEmpty() {return generatedEmptyMesh;}
        bool isOnFrustum(PerspectiveCamera& cam) const;

        Block* getBlock(uint32_t x, uint32_t y, uint32_t z);
        bool setBlock(uint32_t x, uint32_t y, uint32_t z, Block value);
        
        /*
            Launches a thread if possible, the when the mesh is generated sends it to the worlds mesh loading queue,

            ISNT RESPONSIBLE FOR ACTUALLY UPLOADING THE MESH
        */
        void asyncGenerateAsyncUploadMesh(ThreadPool& pool, bool reload);

        /*
            When the mesh is generated sends it to the worlds mesh loading queue,

            ISNT RESPONSIBLE FOR ACTUALLY UPLOADING THE MESH
        */
        void syncGenerateAsyncUploadMesh();
        
        /*
            Generates and uploads the newly generated chunk mesh right away
        */
        void syncGenerateSyncUploadMesh(ChunkMeshRegistry& buffer);

        World& getWorld();
        const glm::ivec3& getWorldPosition(){
            return this->worldPosition;
        }

        bool isMainGroupOfSize(int size){return currentGroup && currentGroup->getSize() == size;}

        std::unique_ptr<DynamicChunkContents>& getMainGroup() {return currentGroup;}
        void setMainGroup(std::unique_ptr<DynamicChunkContents> group) {this->currentGroup = std::move(group);}
}; 

struct Face{
    int x;
    int y;
    int width;
    int height;
};

//std::vector<Face> greedyMeshPlane64(std::array<uint64_t, 64> rows);

#include <game/world.hpp>

#endif