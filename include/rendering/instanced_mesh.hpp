#pragma once

#include <glad/glad.h>
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <coherency.hpp>
#include <rendering/opengl/buffer.hpp>
#include <memory>

class InstancedMesh{
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
        const static size_t instance_data_size = 12;

    private:
        std::array<std::vector<float>, 4> instance_data{};
    
    public:
        InstancedMesh(){}
        void addQuadFace(glm::vec3 position, float width, float height, int texture_index, FaceType type, Direction direction, const std::array<float, 4>& occlusion);
        void preallocate(size_t size, FaceType type);
        const std::vector<float>& getInstanceData(FaceType type);
        bool empty();
        void shrink();
};

class InstancedMeshBuffer{
    public:
        class LoadedMesh{
            private:
                InstancedMeshBuffer& creator;
                bool valid = true;
                std::array<CoherentList<float>::RegionIterator, 4> loaded_regions = {}; 
                std::array<bool, 4> has_region = {};

                friend class InstancedMeshBuffer;

            public:
                LoadedMesh(InstancedMeshBuffer& creator): creator(creator) {}

                //~LoadedMesh() {destroy();}
                // Adds the meshes draw call to the next batch
                void addDrawCall();
                void render();
                void update(InstancedMesh& mesh);
            void destroy();
                bool isValid(){return valid;}
        };
        
    private:
        static const size_t distinct_face_count = 4;

        struct RenderableGroup{
            GLCoherentBuffer<float, GL_ARRAY_BUFFER> instance_data{};
            GLDrawCallBuffer draw_call_buffer{};
            GLVertexArray vao{};
        };
        
        std::array<RenderableGroup, distinct_face_count> render_information{};
        GLBuffer<float, GL_ARRAY_BUFFER> loaded_face_buffer{};

        void removeMesh(LoadedMesh& mesh);
        void addDrawCall(LoadedMesh& mesh);
        void renderMesh(LoadedMesh& mesh);
        void updateMesh(LoadedMesh& loaded_mesh, InstancedMesh& new_mesh);
                
    public:
        InstancedMeshBuffer();

        std::unique_ptr<LoadedMesh> loadMesh(InstancedMesh& mesh);
        void render();
        void clearDrawCalls();
        void flushDrawCalls();
};
