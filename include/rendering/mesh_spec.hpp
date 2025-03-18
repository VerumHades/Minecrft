#pragma once

#include <cstdlib>
#include <glm/glm.hpp>

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

class LoadedMeshInterface{
    public:
        virtual void addDrawCall() = 0;
        virtual void render() = 0;
        virtual void update(MeshInterface& mesh) = 0;
        virtual void destroy() = 0;
        virtual bool isValid() = 0;
};

class MeshLoaderInterface{
    public:
        virtual std::unique_ptr<LoadedMeshInterface> loadMesh(MeshInterface& mesh) = 0;
        virtual void render() = 0;
        virtual void clearDrawCalls() = 0;
        virtual void flushDrawCalls() = 0;
};