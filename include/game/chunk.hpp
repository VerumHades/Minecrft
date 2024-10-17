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

#define CHUNK_SIZE 64

class World;

template <typename T>
struct ChunkMask{
    Block block = {BlockTypes::Air};
    BitArray3D<T> segments = {}; 
    BitArray3D<T> segmentsRotated = {}; 
    
    void set(uint32_t x,uint32_t y,uint32_t z) {
        segments[z][y] |= (1_uint64 << (BitArray3D<T>::T_bits_total - 1 - x));
        segmentsRotated[x][y] |= (1_uint64 << (BitArray3D<T>::T_bits_total - 1 - z));
    }
    void reset(uint32_t x,uint32_t y,uint32_t z) {
        segments[z][y] &= ~(1_uint64 << (BitArray3D<T>::T_bits_total - 1 - x));
        segmentsRotated[x][y] &= ~(1_uint64 << (BitArray3D<T>::T_bits_total - 1 - z));
    }
};

class Chunk: public Volume{
    private:
        glm::vec3 worldPosition = glm::vec3(0,0,0);
        World& world; 

        //Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE] = {};
        ChunkMask<uint64_f> solidMask;
        std::unordered_map<BlockTypes,ChunkMask<uint64_f>> masks;
        //unsigned char lightArray[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE][3];
        //std::unique_ptr<ChunkTreeNode> rootNode = std::make_unique<ChunkTreeNode>();
        
    public:
        bool reloadMesh = false;
        bool pendingUpload = false;

        bool meshGenerating = false;
        bool meshGenerated = false;
        
        std::unique_ptr<Mesh> solidMesh;
        //std::optional<Mesh> transparentMesh;

        bool isDrawn = false;
        bool isEmpty() const {return masks.size() == 0;}
        bool isOnFrustum(PerspectiveCamera& cam) const;

        Chunk(World& world, const glm::vec3& pos);

        Block* getBlock(uint32_t x, uint32_t y, uint32_t z);
        bool setBlock(uint32_t x, uint32_t y, uint32_t z, Block value);

        void generateMeshes();

        World& getWorld();
        const glm::vec3& getWorldPosition(){
            return this->worldPosition;
        }

        ChunkMask<uint64_f>& getSolidMask() {return solidMask;}
        std::unordered_map<BlockTypes,ChunkMask<uint64_f>>& getMasks() {return masks;}

}; 
extern std::unordered_map<BlockTypes, BlockType> predefinedBlocks;
extern std::mutex predefinedBlockMutex;

inline const BlockType& getBlockType(Block* block){
    if (block->type < BlockTypes::Air || block->type > BlockTypes::Sand) {
        std::cerr << "Error: Invalid BlockTypes value: " << static_cast<int>(block->type) << std::endl;
        std::terminate(); 
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

//std::vector<Face> greedyMeshPlane64(std::array<uint64_f, 64> rows);

#include <game/world.hpp>

#endif