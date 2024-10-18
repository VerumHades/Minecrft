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

template <typename T>
struct ChunkMaskGroup{
    ChunkMask<T> solidMask;
    std::unordered_map<BlockTypes,ChunkMask<T>> masks;
};

class Chunk: public Volume{
    private:
        glm::ivec3 worldPosition = glm::ivec3(0,0,0);
        World& world; 

        enum Resolution{
            RES_8,
            RES_16,
            RES_32,
            RES_64
        };
        Resolution lodResolution = RES_64;

        std::unique_ptr<ChunkMaskGroup<uint_fast8_t >> maskGroup8 ;
        std::unique_ptr<ChunkMaskGroup<uint_fast16_t>> maskGroup16;
        std::unique_ptr<ChunkMaskGroup<uint_fast32_t>> maskGroup32;
        std::unique_ptr<ChunkMaskGroup<uint_fast64_t>> maskGroup64;
        //Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE] = {};
        ChunkMaskGroup<uint64_f> mainGroup;
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

        bool isEmpty() const {return mainGroup.masks.size() == 0;}
        bool isOnFrustum(PerspectiveCamera& cam) const;

        Chunk(World& world, const glm::vec3& pos);

        Block* getBlock(uint32_t x, uint32_t y, uint32_t z);
        bool setBlock(uint32_t x, uint32_t y, uint32_t z, Block value);
        
        void generateMesh(MultiChunkBuffer& buffer, ThreadPool& pool);
        void updateMesh(); // Marks the mesh for updating

        World& getWorld();
        const glm::ivec3& getWorldPosition(){
            return this->worldPosition;
        }

        ChunkMask<uint64_f>& getSolidMask() {return mainGroup.solidMask;}
        std::unordered_map<BlockTypes,ChunkMask<uint64_f>>& getMasks() {return mainGroup.masks;}

}; 

struct Face{
    int x;
    int y;
    int width;
    int height;
};

//std::vector<Face> greedyMeshPlane64(std::array<uint64_f, 64> rows);

#include <game/world.hpp>

#endif