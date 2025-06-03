#pragma once


#include <structure/segregated_list.hpp>

#include <rendering/opengl/shaders.hpp>
#include <rendering/mesh_spec.hpp>
#include <rendering/opengl/buffer.hpp>

#include <glm/glm.hpp>
#include <coherency.hpp>
#include <array>
/*
    Vertex format:

    (Indexed from the left)
    
    uint32
    (0 ) -> 6 bits x
    (6 ) -> 7 bits y
    (13) -> 6 bits z
    (19) -> 6 bits width
    (25) -> 6 bits height
    (31) -> 1 bit for direction (forward/backward)

    uint32
    (0 ) -> 4 * 2 bits occlusion
    (8 ) -> 24 bits texture index
*/
class PooledMesh: public MeshInterface{
    private:
        SegregatedList<FaceType, uint32_t> data{};

    public:
        const static int face_size = 2;

        PooledMesh(){}
        void addQuadFace(const glm::ivec3& position, float width, float height, int texture_index, FaceType type, Direction direction, const std::array<float, 4>& occlusion, const glm::vec3& world_position) override;
        void preallocate(size_t size, FaceType type) override;
        const SegregatedList<FaceType, uint32_t>& GetData() { return data; };
        bool empty() override;
        void shrink() override;
};

class PooledMeshLoader: public MeshLoaderInterface{
    public:
        class LoadedMesh: public LoadedMeshInterface{
            private:
                PooledMeshLoader& creator;
                bool valid = true;
                std::array<CoherentList<uint32_t>::RegionIterator, 4> loaded_regions = {}; 
                std::array<bool, 4> has_region = {};

                friend class PooledMeshLoader;

            public:
                LoadedMesh(PooledMeshLoader& creator): creator(creator) {}

                //~LoadedMesh() {destroy();}
                // Adds the meshes draw call to the next batch
                void addDrawCall(const glm::ivec3& position) override;
                void update(MeshInterface* mesh) override;
                void destroy() override;
                bool isValid() override {return valid;}
        };
        
    private:
        static const size_t distinct_face_count = 3;
        static ShaderProgram& GetProgram(){
            static std::unique_ptr<ShaderProgram> program = nullptr;
            if(!program){
                program = std::make_unique<ShaderProgram>("resources/shaders/graphical/vertex_pooling/pooling.vs","resources/shaders/graphical/vertex_pooling/pooling.fs");
                program->setSamplerSlot("textureArray", 0);
                program->setSamplerSlot("shadowMap", 1);
            }
            return *program;
        }

        Uniform<float> face_type_uniform = Uniform<float>("FaceType");

        bool legacy_mode = true;

        GLVertexArray dummy_vao;

        GLBuffer<int32_t, GL_SHADER_STORAGE_BUFFER> world_position_buffer;
        std::vector<glm::ivec3> world_positions = {};

        struct LegacyCall{
            std::array<int, distinct_face_count> starts;
            std::array<GLsizei, distinct_face_count> counts;
            glm::ivec3 world_position;
        };

        std::vector<LegacyCall> legacy_calls;

        bool updated_world_positions = false;

        struct RenderableGroup{
            GLCoherentBuffer<uint32_t, GL_SHADER_STORAGE_BUFFER> mesh_data{};
            GLDrawCallBuffer draw_call_buffer{};

            std::vector<int> draw_starts = {};
            std::vector<GLsizei> draw_sizes = {};
        };

        Uniform<glm::ivec3> world_position =  Uniform<glm::ivec3>("world_position");


        std::array<RenderableGroup, distinct_face_count> render_information{};

        void removeMesh(LoadedMesh& mesh);
        void addDrawCall(LoadedMesh& mesh, const glm::ivec3& position);
        void updateMesh(LoadedMesh& loaded_mesh, PooledMesh& new_mesh);
                
    public:
        PooledMeshLoader();

        std::unique_ptr<LoadedMeshInterface> loadMesh(MeshInterface* mesh) override;
        void render() override;
        void clearDrawCalls() override;
        void flushDrawCalls() override;
        bool DrawFailed() override {return false;}
};
