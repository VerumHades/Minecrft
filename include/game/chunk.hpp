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
#include <blockarray.hpp>

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

class Chunk: public SparseBlockArray{
    private:
        glm::ivec3 worldPosition;

    public:
        Chunk(glm::ivec3 worldPosition, BlockRegistry& blockRegistry): SparseBlockArray(blockRegistry), worldPosition(worldPosition) { }

        const glm::ivec3& getWorldPosition() const { return worldPosition; }
        
        friend class ChunkMeshGenerator;
        friend class WorldGenerator;
        //std::optional<Mesh> transparentMesh;
}; 

#endif