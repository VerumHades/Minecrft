#ifndef CHUNK_H
#define CHUNK_H

#include <cmath>
#include <ctime>

#include <rendering/opengl/texture.hpp>
#include <rendering/opengl/buffer.hpp>
#include <rendering/region_culler.hpp>
#include <rendering/mesh.hpp>
#include <rendering/camera.hpp>

#include <structure/serialization/serializer.hpp>
#include <structure/bitworks.hpp>

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

class TerrainManager;

/**
 * @brief An array of blocks with a world position and a simplification level for its mesh
 * 
 */
class Chunk: public SparseBlockArray{
    private:
        glm::ivec3 worldPosition;
        BitField3D::SimplificationLevel current_simplification = BitField3D::NONE;

        friend class ChunkMeshGenerator;
        friend class WorldGenerator;
        friend class TerrainManager;

        friend class Serializer;

    public:
        Chunk(): Chunk({0,0,0}){}
        Chunk(glm::ivec3 worldPosition): SparseBlockArray(), worldPosition(worldPosition) { }

        const glm::ivec3& getWorldPosition() const { return worldPosition; }
        void setWorldPosition(const glm::ivec3& position){ worldPosition = position; }
};

#endif
