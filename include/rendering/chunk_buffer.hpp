#pragma once

#include <vec_hash.hpp>

#include <rendering/instanced_mesh.hpp>
#include <rendering/allocator.hpp>
#include <rendering/opengl/buffer.hpp>
#include <rendering/culling.hpp>
#include <rendering/wireframes.hpp>


#include <sstream>    
#include <iomanip>    
#include <string>    
#include <bitset>

struct TransformHash;
struct TransformEqual;
class ChunkMeshRegistry;

class MeshRegion{
    private:
        struct Transform{
            /*
                Position within its level, actual position is 2^level * position
            */
            glm::ivec3 position = {0,0,0};
            /*
                Smallest is 1.
                The actuall size of a region based on its level is: 2^level
            */
            uint level = 1;
        } transform;

        ChunkMeshRegistry& registry;
        MeshRegion(Transform& transform, ChunkMeshRegistry& registry): transform(transform), registry(registry) {}

        std::unique_ptr<InstancedMeshBuffer::LoadedMesh> loaded_mesh;

        /*
            Returns a pointer to the regions parent, if the region has no parents return nullptr
        */
        MeshRegion* getParentRegion();
        /*
            Returns a pointer to the subregion in relative coordinates to the parent

            return nullptr if the region is at level 1 (there are no levels under 1).
        */
        MeshRegion* getSubregion(uint x, uint y, uint z);
        /*
            Returns a position relative to the parent

            will return {0,0,0} if no parent is present
        */
        glm::ivec3 getParentRelativePosition();

        /*
            Returns the position of the regions parent
        */
        glm::ivec3 getParentPosition() {
            return {
                std::floor(static_cast<float>(transform.position.x) / 2.0f),
                std::floor(static_cast<float>(transform.position.y) / 2.0f),
                std::floor(static_cast<float>(transform.position.z) / 2.0f)
            };
        }

        friend struct TransformHash;
        friend struct TransformEqual;
        friend class ChunkMeshRegistry;

};

struct TransformHash{
    std::size_t operator()(const MeshRegion::Transform& transform) const {
        std::size_t h1 = std::hash<float>{}(transform.position.x);
        std::size_t h2 = std::hash<float>{}(transform.position.y);
        std::size_t h3 = std::hash<float>{}(transform.position.z);
        std::size_t h4 = std::hash<int>{}(transform.level);

        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

struct TransformEqual {
    bool operator()(const MeshRegion::Transform& a, const MeshRegion::Transform& b) const {
        return a.position == b.position && a.level == b.level;
    }
};

class ChunkMeshRegistry{
    private:
        InstancedMeshBuffer mesh_buffer;

        // Highest region level, no regions higher than maxRegionLevel will be registered or created
        const static uint maxRegionLevel = 5;
        
        // index 0 is level 1, precalculated sizes
        std::vector<uint> actualRegionSizes;

        std::unordered_map<MeshRegion::Transform, MeshRegion, TransformHash, TransformEqual> regions;

        /*
            Checks the region againist the frustum. Does an octree search among its children.
            Ignores region that arent drawn.
            Writes their calls directly into the draw call buffer begining at the draw call index.
        */
        void processRegionForDrawing(Frustum& frustum, MeshRegion* region);

    public:
        void initialize(uint renderDistance);

        void clear();
        bool isChunkLoaded(const glm::ivec3& pos){
            return getRegion({pos,1}) != nullptr;
        }

        /*
            Updates draw calls to the ones visible in the frustum.

            Takes the camera position in world coordinates
        */
        void updateDrawCalls(glm::ivec3 camera_position, Frustum& frustum);

        /*
            Uploads the mesh as a level 1 region. (All parents are automatically created if they dont already exist)

            Empty meshes will be ingnored. But registered as empty regions.
        */
        bool addMesh(InstancedMesh* mesh, const glm::ivec3& pos);

        /*
            Updates a mesh, splits regions if old mesh is a part of them
        */
        bool updateMesh(InstancedMesh* mesh, const glm::ivec3& pos);

        /*
            Creates a region and all its parents if they dont already exist.

            returns a `nullptr` on failure
        */
        MeshRegion* createRegion(MeshRegion::Transform transform);

        MeshRegion* getRegion(MeshRegion::Transform transform) {
            if(!regions.contains(transform)) return nullptr;
            return &regions.at(transform);
        }

        /*
            Return the real size of a region based on its level
        */
        int getRegionSizeForLevel(uint level) {
            if(level < 1 || level > maxRegionLevel) return 0;
            return actualRegionSizes[level - 1];
        }

        void draw();
};

#include <rendering/mesh.hpp>