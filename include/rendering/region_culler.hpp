#pragma once

#include <vec_hash.hpp>

#include <rendering/culling.hpp>
#include <rendering/mesh_spec.hpp>

#include <sstream>    
#include <iomanip>    
#include <string>    
#include <bitset>
#include <mutex>

struct TransformHash;
struct TransformEqual;
class RegionCuller;


/**
 * @brief A meshable region
 * 
 */
class MeshRegion{
    private:
        struct Transform{
            /**
            * @brief Position within its level, actual position is 2^level * position
            * 
            */
            glm::ivec3 position = {0,0,0};
            /**
             * @brief  Smallest is 1. The actuall size of a region based on its level is: 2^level
             * 
             */
            uint level = 1;
        } transform;

        std::bitset<8> child_registry;

        RegionCuller& registry;
        MeshRegion(Transform& transform, RegionCuller& registry): transform(transform), registry(registry) {}

        std::unique_ptr<LoadedMeshInterface> loaded_mesh = nullptr;

        void SetChild(uint x, uint y, uint z, bool value);

        /**
         * @brief Returns a pointer to the regions parent, if the region has no parents return nullptr
         * 
         * @return MeshRegion* 
         */
        MeshRegion* getParentRegion();
        /**
         * @brief Returns a pointer to the subregion in relative coordinates to the parent
         * 
         * @param x 
         * @param y 
         * @param z 
         * @return MeshRegion* 
         */
        MeshRegion* getSubregion(uint x, uint y, uint z);
        /**
         * @brief Returns a position relative to the parent
         * 
         * @return glm::ivec3 
         */
        glm::ivec3 getParentRelativePosition();

        /**
         * @brief Returns the position of the regions parent
         * 
         * @return glm::ivec3 
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
        friend class RegionCuller;

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

class RegionCuller{
    private:
        // Highest region level, no regions higher than maxRegionLevel will be registered or created
        const static uint maxRegionLevel = 5;
        
        // index 0 is level 1, precalculated sizes
        std::vector<uint> actualRegionSizes;

        std::unordered_map<MeshRegion::Transform, MeshRegion, TransformHash, TransformEqual> regions;

        /**
         * @brief Checks the region againist the frustum. Does an octree search among its children.
         * Ignores region that arent drawn.
         * Adds drawcalls automatically
         * @param frustum 
         * @param region 
         */
        void processRegionForDrawing(Frustum& frustum, MeshRegion* region);

        MeshLoaderInterface* mesh_loader = nullptr;

        std::mutex mesh_change_mutex;

        bool updateMeshUnguarded(MeshInterface* mesh, const glm::ivec3& pos);

    public:
        RegionCuller();

        void SetMeshLoader(MeshLoaderInterface* loader){ mesh_loader = loader; }

        void clear();
        bool isChunkLoaded(const glm::ivec3& pos){
            return getRegion({pos,1}) != nullptr;
        }

        /**
         * @brief Updates draw calls to the ones visible in the frustum.
         * 
         * @param camera_position world position of camera
         * @param frustum camera frustum
         */
        void updateDrawCalls(const glm::ivec3& camera_position, Frustum& frustum);

        /**
         * @brief Uploads the mesh as a level 1 region. (All parents are automatically created if they dont already exist)
         * 
         * @param mesh 
         * @param pos 
         * @return true 
         * @return false 
         */
        bool addMesh(MeshInterface* mesh, const glm::ivec3& pos);

        /**
         * @brief Updates a mesh, splits regions if old mesh is a part of them
         * 
         * @param mesh 
         * @param pos 
         * @return true 
         * @return false 
         */
        bool updateMesh(MeshInterface* mesh, const glm::ivec3& pos);

        bool removeMesh(const glm::ivec3& pos);

        /**
         * @brief Creates a region and all its parents if they dont already exist.
         * 
         * @param transform 
         * @return MeshRegion* 
         */
        MeshRegion* createRegion(MeshRegion::Transform transform);

        MeshRegion* getRegion(MeshRegion::Transform transform) {
            if(!regions.contains(transform)) return nullptr;
            return &regions.at(transform);
        }

        /**
         * @brief Destroys a region
         * 
         * @param transform 
         */
        void removeRegion(MeshRegion::Transform transform){
            auto* region = getRegion(transform);
            if(!region) return;
            
            auto* parent = region->getParentRegion();
            if(parent){
                auto pos = region->getParentRelativePosition();
                parent->SetChild(pos.x,pos.y,pos.z, false);
            }

            regions.erase(transform);
        }

        /**
         * @brief Return the real size of a region based on its level
         * 
         * @param level 
         * @return int 
         */
        int getRegionSizeForLevel(uint level) {
            if(level < 1 || level > maxRegionLevel) return 0;
            return actualRegionSizes[level - 1];
        }

        void draw();
};

#include <rendering/mesh.hpp>