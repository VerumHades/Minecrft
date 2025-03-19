#pragma once

#include <glad/glad.h>
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <coherency.hpp>
#include <rendering/opengl/buffer.hpp>
#include <rendering/opengl/shaders.hpp>
#include <memory>
#include <rendering/mesh_spec.hpp>

class InstancedMesh: public MeshInterface{
    public:
        const static size_t instance_data_size = 12;

    private:
        std::array<std::vector<float>, 4> instance_data{};
    
    public:
        InstancedMesh(){}
        void addQuadFace(const glm::vec3& position, float width, float height, int texture_index, FaceType type, Direction direction, const std::array<float, 4>& occlusion) override;
        void preallocate(size_t size, FaceType type) override;
        const std::vector<float>& getInstanceData(FaceType type);
        bool empty() override;
        void shrink() override;
};

class InstancedMeshLoader: public MeshLoaderInterface{
    public:
        class LoadedMesh: public LoadedMeshInterface{
            private:
                InstancedMeshLoader& creator;
                bool valid = true;
                std::array<CoherentList<float>::RegionIterator, 4> loaded_regions = {}; 
                std::array<bool, 4> has_region = {};

                friend class InstancedMeshLoader;

            public:
                LoadedMesh(InstancedMeshLoader& creator): creator(creator) {}

                //~LoadedMesh() {destroy();}
                // Adds the meshes draw call to the next batch
                void addDrawCall(const glm::ivec3& position) override ;
                void render();
                void update(MeshInterface* mesh) override ;
                void destroy() override ;
                bool isValid() override {return valid;}
        };
        
    private:
        static const size_t distinct_face_count = 4;
        bool draw_failed = false;

        ShaderProgram shared_program = ShaderProgram("resources/shaders/terrain.vs","resources/shaders/terrain.fs");

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
        InstancedMeshLoader();

        std::unique_ptr<LoadedMeshInterface> loadMesh(MeshInterface* mesh) override;
        void render() override;
        void clearDrawCalls() override;
        void flushDrawCalls() override;
        bool DrawFailed() override { return draw_failed; }
};
