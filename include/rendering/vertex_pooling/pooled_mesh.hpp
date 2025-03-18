#pragma once

#include <rendering/mesh_spec.hpp>
#include <structure/segregated_list.hpp>
#include <rendering/opengl/buffer.hpp>
#include <glm/glm.hpp>
#include <coherency.hpp>

/*
    Vertex format:

    (Indexed from the left)
    
    uint32
    (0 ) -> 6 bits x
    (6 ) -> 6 bits y
    (12) -> 6 bits z
    (18) -> 6 bits width
    (24) -> 6 bits height
    (30) -> 1 bit for direction (forward/backward)
    (31) -> 1 bit left

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
        void addQuadFace(const glm::vec3& position, float width, float height, int texture_index, FaceType type, Direction direction, const std::array<float, 4>& occlusion) override;
        void preallocate(size_t size, FaceType type) override;
        const SegregatedList<FaceType, uint32_t>& GetData() { return data; };
        bool empty() override;
        void shrink() override;
};

class PooledMeshRegistry{
    public:
        class LoadedMesh{
            private:
                PooledMeshRegistry& creator;
                bool valid = true;
                std::array<CoherentList<uint32_t>::RegionIterator, 4> loaded_regions = {}; 
                std::array<bool, 4> has_region = {};

                friend class PooledMeshRegistry;

            public:
                LoadedMesh(PooledMeshRegistry& creator): creator(creator) {}

                //~LoadedMesh() {destroy();}
                // Adds the meshes draw call to the next batch
                void addDrawCall(const glm::ivec3& position);
                void update(PooledMesh& mesh);
            void destroy();
                bool isValid(){return valid;}
        };
        
    private:
        static const size_t distinct_face_count = 4;

        GLBuffer<int32_t, GL_SHADER_STORAGE_BUFFER> world_position_buffer;
        
        std::vector<int32_t> world_positions = {};
        bool updated_world_positions = false;

        struct RenderableGroup{
            GLCoherentBuffer<uint32_t, GL_SHADER_STORAGE_BUFFER> mesh_data{};

            std::vector<int> draw_starts = {};
            std::vector<GLsizei> draw_sizes = {};
        };


        std::array<RenderableGroup, distinct_face_count> render_information{};

        void removeMesh(LoadedMesh& mesh);
        void addDrawCall(LoadedMesh& mesh, const glm::ivec3& position);
        void updateMesh(LoadedMesh& loaded_mesh, PooledMesh& new_mesh);
                
    public:
        PooledMeshRegistry();

        std::unique_ptr<LoadedMesh> loadMesh(PooledMesh& mesh);
        void render();
        void clearDrawCalls();
        void flushDrawCalls();
};
