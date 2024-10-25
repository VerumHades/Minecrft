#ifndef CHUNK_H
#define CHUNK_H

#include <cmath>
#include <ctime>

#include <rendering/texture.hpp>
#include <rendering/buffer.hpp>
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
        //Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE] = {};
        std::unique_ptr<DynamicChunkMaskGroup> currentGroup = {};
        //unsigned char lightArray[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE][3];
        //std::unique_ptr<ChunkTreeNode> rootNode = std::make_unique<ChunkTreeNode>();
        
        void generateMeshes();

        bool generatedEmptyMesh = false;
        bool reloadMesh = false;
        bool pendingUpload = false;

        bool meshGenerating = false;
        bool meshGenerated = false;
        
        std::unique_ptr<Mesh> solidMesh;
        
    public:
        //std::optional<Mesh> transparentMesh;

        bool isEmpty() const {return currentGroup && currentGroup->empty();}
        bool isMeshEmpty() {return generatedEmptyMesh;}
        bool isOnFrustum(PerspectiveCamera& cam) const;
        bool needsMeshReload(){return reloadMesh;}

        Chunk(World& world, const glm::vec3& pos);

        Block* getBlock(uint32_t x, uint32_t y, uint32_t z);
        bool setBlock(uint32_t x, uint32_t y, uint32_t z, Block value);
        
        void generateMesh(MultiChunkBuffer& buffer, ThreadPool& pool);
        void updateMesh(); // Marks the mesh for updating

        World& getWorld();
        const glm::ivec3& getWorldPosition(){
            return this->worldPosition;
        }

        bool isMainGroupOfSize(int size){return currentGroup && currentGroup->getSize() == size;}

        template <int size>
        ChunkMaskGroup<size>* getMainGroupAs() { return static_cast<ChunkMaskGroup<size>*>(currentGroup.get()); }

        std::unique_ptr<DynamicChunkMaskGroup>& getMainGroup() {return currentGroup;}
        void setMainGroup(std::unique_ptr<DynamicChunkMaskGroup> group) {this->currentGroup = std::move(group);}
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