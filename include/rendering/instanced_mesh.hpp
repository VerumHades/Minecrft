#pragma once

#include <glad/glad.h>
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <coherency.hpp>
#include <rendering/opengl/buffer.hpp>

class InstancedMesh{
    public:
        enum FaceType{
            X_ALIGNED = 0,
            Y_ALIGNED = 1,
            Z_ALIGNED = 2,
            BILBOARD  = 3
        };

        enum Direction{
            Forward = 1,
            Backward = -1
        };
        const static size_t instance_data_size = 10;

    private:
        std::array<std::vector<float>, 4> instance_data;
    
    public:
        void addQuadFace(glm::vec3 position, float width, float height, int texture_index, FaceType type, Direction direction);
        const std::vector<float>& getInstanceData(FaceType type);
};

class InstancedMeshBuffer{
    public:
        struct LoadedMesh{
            std::array<CoherentList<float>::RegionIterator, 4> loaded_regions; 
        };
        
        using LoadedMeshIterator = std::list<LoadedMesh>::iterator;
    private:
        static const size_t distinct_face_count = 4;

        std::list<LoadedMesh> loadedMeshes;

        std::array<CoherentList<float>, distinct_face_count> instance_data;

        std::array<GLVertexArray, distinct_face_count> vaos;
        std::array<GLBuffer<float, GL_ARRAY_BUFFER>, distinct_face_count> instance_buffers;
        std::array<GLBuffer<float, GL_ARRAY_BUFFER>, distinct_face_count> loaded_face_buffers;

    public:
        InstancedMeshBuffer();

        void renderMesh(LoadedMeshIterator iterator);
        LoadedMeshIterator loadMesh(InstancedMesh& mesh);
        void removeMesh(LoadedMeshIterator iterator);
};
