#ifndef CHUNK_H
#define CHUNK_H

#include <cmath>
#include <ctime>

#include <rendering/texture.hpp>
#include <rendering/buffer.hpp>
#include <rendering/mesh.hpp>
#include <rendering/camera.hpp>
#include <rendering/model.hpp>
#include <rendering/compression.hpp>

#include <glm/glm.hpp>
#include <map>
#include <optional>
#include <functional>
#include <mutex>
#include <memory>
#include <array>
#include <blocks.hpp>

#include <bit>
#include <bitset>
#include <iostream>

#define CHUNK_SIZE 64

class World;

struct ChunkMask{
    Block block = {BlockTypes::Air};
    BitArray3D segments = {}; 
    BitArray3D segmentsRotated = {}; 
    
    void set(uint32_t x,uint32_t y,uint32_t z) {
        segments[z][y] |= (1ULL << (63 - x));
        segmentsRotated[x][y] |= (1ULL << (63 - z));
    }
    void reset(uint32_t x,uint32_t y,uint32_t z) {
        segments[z][y] &= ~(1ULL << (63 - x));
        segmentsRotated[x][y] &= ~(1ULL << (63 - z));
    }
};

class Chunk: public Volume{
    private:
        glm::vec3 worldPosition = glm::vec3(0,0,0);
        World& world;

        //Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE] = {};
        ChunkMask solidMask;
        std::unordered_map<BlockTypes,ChunkMask> masks;
        //unsigned char lightArray[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE][3];
        //std::unique_ptr<ChunkTreeNode> rootNode = std::make_unique<ChunkTreeNode>();
        
    public:
        bool meshGenerating = false;
        bool meshGenerated = false;

        bool buffersLoaded = false;
        
        std::unique_ptr<Mesh> solidMesh;
        //std::optional<Mesh> transparentMesh;

        bool isDrawn = false;
        bool isEmpty = false;
        bool isOnFrustum(PerspectiveCamera& cam) const;

        Chunk(World& world, const glm::vec3& pos);

        Block* getBlock(uint32_t x, uint32_t y, uint32_t z);
        bool setBlock(uint32_t x, uint32_t y, uint32_t z, Block value);

        void generateMeshes();

        World& getWorld();
        const glm::vec3& getWorldPosition(){
            return this->worldPosition;
        }

        ChunkMask& getSolidMask() {return solidMask;}
        std::unordered_map<BlockTypes,ChunkMask>& getMasks() {return masks;}

}; 
extern std::unordered_map<BlockTypes, BlockType> predefinedBlocks;
extern std::mutex predefinedBlockMutex;

inline const BlockType& getBlockType(Block* block){
    if (block->type < BlockTypes::Air || block->type > BlockTypes::Sand) {
        std::cerr << "Error: Invalid BlockTypes value: " << static_cast<int>(block->type) << std::endl;
        std::terminate(); // Or handle the error appropriately
    }
    
    //std::lock_guard<std::mutex> lock(predefinedBlockMutex);
    return predefinedBlocks[block->type];
}


std::string getBlockTypeName(BlockTypes type);

struct Face{
    int x;
    int y;
    int width;
    int height;
};

//std::vector<Face> greedyMeshPlane64(std::array<uint64, 64> rows);

#include <world.hpp>

#endif