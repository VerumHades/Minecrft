#pragma once

class MeshIterface{
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
