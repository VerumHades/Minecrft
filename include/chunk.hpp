#ifndef CHUNK_H
#define CHUNK_H

#include <cmath>
#include <ctime>


#include <rendering/texture.hpp>
#include <rendering/buffer.hpp>
#include <rendering/mesh.hpp>
#include <rendering/camera.hpp>
#include <entity.hpp>

#include <glm/glm.hpp>
#include <map>
#include <optional>
#include <functional>
#include <mutex>
#include <memory>
#include <array>
#include <blocks.hpp>

#define OK -1
#define INVALID_COORDINATES -2
#define LAYER_CORRUPTED -3

#define DEFAULT_CHUNK_SIZE 16
#define DEFAULT_CHUNK_AREA DEFAULT_CHUNK_SIZE * DEFAULT_CHUNK_SIZE
#define DEFAULT_CHUNK_HEIGHT 256

struct FaceDefinition {
    int offsetX = 0;
    int offsetY = 0;
    int offsetZ = 0;
    std::vector<int> vertexIndexes;
    int textureIndex = 0;
    bool clockwise = false;

    FaceDefinition(int offsetX, int offsetY, int offsetZ, 
                   std::vector<int> vertexIndexes, int textureIndex, 
                   bool clockwise = false)
        : offsetX(offsetX), offsetY(offsetY), offsetZ(offsetZ),
          vertexIndexes(std::move(vertexIndexes)), textureIndex(textureIndex),
          clockwise(clockwise) {}
};


class World;

class Chunk: public Volume{
    private:
        glm::vec2 worldPosition = glm::vec2(0,0);
        World& world;

        Block blocks[DEFAULT_CHUNK_SIZE][DEFAULT_CHUNK_HEIGHT][DEFAULT_CHUNK_SIZE] = {};
        //unsigned char lightArray[DEFAULT_CHUNK_SIZE][DEFAULT_CHUNK_HEIGHT][DEFAULT_CHUNK_SIZE][3];
        
    public:
        bool meshGenerating = false;
        bool meshGenerated = false;

        bool buffersLoaded = false;
        
        std::unique_ptr<GLDoubleBuffer> solidBuffer;
        std::unique_ptr<Mesh> solidMesh;
        //std::optional<Mesh> transparentMesh;

        bool isDrawn;
        bool isOnFrustum(PerspectiveCamera& cam) const;

        Chunk(World& world, const glm::vec2& pos);

        Block* getBlock(unsigned int x, unsigned int y, unsigned int z);
        bool setBlock(unsigned int x, unsigned int y, unsigned int z, Block value);

        void generateMeshes();
        void regenerateMesh();
        void regenerateMesh(glm::vec2 blockCoords);

        World& getWorld();
        const glm::vec2& getWorldPosition(){
            return this->worldPosition;
        }

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

#include <world.hpp>

#endif