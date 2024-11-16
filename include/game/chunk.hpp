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


class Chunk{
    private:
        glm::ivec3 worldPosition;
        std::unique_ptr<DynamicChunkContents> currentGroup;

    public:
        Chunk(glm::ivec3 worldPosition): worldPosition(worldPosition){ }

        const glm::ivec3& getWorldPosition() const { return worldPosition; }
        //std::optional<Mesh> transparentMesh;

        bool isEmpty() {return currentGroup && currentGroup->empty();}

        Block* getBlock(uint x, uint y, uint z);
        bool setBlock(uint x, uint y, uint z, Block value);

        bool isMainGroupOfSize(int size){return currentGroup && currentGroup->getSize() == size;}

        std::unique_ptr<DynamicChunkContents>& getMainGroup() {return currentGroup;}
        void setMainGroup(std::unique_ptr<DynamicChunkContents> group) {this->currentGroup = std::move(group);}
}; 

#endif