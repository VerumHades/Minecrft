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
            BILLBOARD  = 3
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
        class LoadedMesh{
            private:
                InstancedMeshBuffer& creator;
                std::array<CoherentList<float>::RegionIterator, 4> loaded_regions; 

                LoadedMesh(InstancedMeshBuffer& creator): creator(creator) {}

                friend class InstancedMeshBuffer;
            public:
                void render();
                void update(InstancedMesh& mesh);
                ~LoadedMesh();
        };
        
    private:
        static const size_t distinct_face_count = 4;

        std::array<CoherentList<float>, distinct_face_count> instance_data;

        std::array<GLVertexArray, distinct_face_count> vaos;
        GLBuffer<float, GL_ARRAY_BUFFER> loaded_face_buffer;
        
        std::array<GLBuffer<float, GL_ARRAY_BUFFER>, distinct_face_count> instance_buffers;

        void removeMesh(LoadedMesh& mesh);
        void renderMesh(LoadedMesh& mesh);
        void updateMesh(LoadedMesh& loaded_mesh, InstancedMesh& new_mesh);
                
    public:
        InstancedMeshBuffer();

        LoadedMesh loadMesh(InstancedMesh& mesh);
};
