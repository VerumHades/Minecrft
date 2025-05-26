#pragma once

#include <cstdlib>
#include <glm/glm.hpp>
#include <memory>

/**
 * @brief A specification of how a block mesh should behave
 * 
 */
class MeshInterface{
    public:
        enum FaceType{
            X_ALIGNED = 0,
            Y_ALIGNED = 1,
            Z_ALIGNED = 2,
            BILLBOARD  = 3
        }; 

        enum Direction{
            Forward = 0,
            Backward = 1
        };

        virtual void addQuadFace(const glm::vec3& position, float width, float height, int texture_index, FaceType type, Direction direction, const std::array<float, 4>& occlusion) = 0;
        virtual void preallocate(size_t size, FaceType type) = 0;
        virtual bool empty() = 0;
        virtual void shrink() = 0;
};

/**
 * @brief A specification of how a loaded mesh should behave
 * 
 */
class LoadedMeshInterface{
    public:
        virtual void addDrawCall(const glm::ivec3& position) = 0;
        virtual void update(MeshInterface* mesh) = 0;
        virtual void destroy() = 0;
        virtual bool isValid() = 0;
};

/**
 * @brief A specification of how a mesh loader should behave
 * 
 */
class MeshLoaderInterface{
    public:
        virtual std::unique_ptr<LoadedMeshInterface> loadMesh(MeshInterface* mesh) = 0;
        virtual void render() = 0;
        virtual void clearDrawCalls() = 0;
        virtual void flushDrawCalls() = 0;
        virtual bool DrawFailed() = 0;
};