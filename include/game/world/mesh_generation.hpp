#pragma once

#include <mutex>
#include <game/world/terrain.hpp>
#include <rendering/mesh_spec.hpp>
#include <rendering/region_culler.hpp>
#include <blockarray.hpp>
#include <glm/glm.hpp>
#include <bit>
#include <atomic>



class ChunkMeshGenerator{
    public:
        struct Face{
            int x;
            int y;
            int width;
            int height;
        };
        struct OccludedPlane{
            std::array<float, 4> occlusion = {0,0,0,0};
            BitPlane<64> plane;

            int start = 0; // Zeroes from the start that can be skipped
            int end = 64; // Zeroes to the bottom
        };

        struct OcclusionPlane{
            std::array<uint64_t, 66> rows{};
            uint64_t left = 0;
            uint64_t right = 0;

            unsigned top_left_corner: 1 = 0;
            unsigned top_right_corner: 1 = 0;
            unsigned bottom_left_corner: 1 = 0;
            unsigned bottom_right_corner: 1 = 0;
        };

    private:
        std::vector<Face>& greedyMeshPlane(BitPlane<64> rows, int start = 0, int end = 64);
        void generateChunkMesh(const glm::ivec3& position, MeshInterface* output, Chunk* group, BitField3D::SimplificationLevel simplification_level);

        std::mutex meshLoadingMutex;
        
        struct MeshLoadingMember{
            glm::ivec3 position;
            std::unique_ptr<InstancedMesh> mesh;
        };

        std::queue<MeshLoadingMember> meshLoadingQueue;

        Terrain* world = nullptr; // Points to the world relative to which you generate meshes, doesnt need to be set

        void addToChunkMeshLoadingQueue(glm::ivec3 position, std::unique_ptr<InstancedMesh> mesh);
        /*
            Creates separate planes from one plane with occlusion values
        */
        std::vector<OccludedPlane>& calculatePlaneAmbientOcclusion(BitPlane<64>& source_plane, OcclusionPlane& occlusion_plane);

        /*
            Returns two plains separated by the occlusion at the offset and information whether they are empty
        */
        std::tuple<OccludedPlane, bool, OccludedPlane, bool> segregatePlane(
            OccludedPlane& source_plane,
            OcclusionPlane& occlusion_plane,
            std::array<bool,4> affects,
            glm::ivec2 lookup_offset
        );

        void proccessOccludedFaces(
            BitPlane<64>& source_plane,
            OcclusionPlane& occlusion_plane,
            
            MeshInterface::FaceType face_type,
            MeshInterface::Direction direction,
            BlockRegistry::BlockPrototype* type,
            MeshInterface* mesh, 
            glm::vec3 world_position,
            int layer
        );

        std::atomic<bool> meshes_pending = false;
    
    public:
        ChunkMeshGenerator(){}
        bool loadMeshFromQueue(RegionCuller&  buffer, size_t limit = 1);

        /*
            When the mesh is generated sends it to the worlds mesh loading queue,

            ISNT RESPONSIBLE FOR ACTUALLY UPLOADING THE MESH
        */
        void syncGenerateAsyncUploadMesh(Chunk* chunk, BitField3D::SimplificationLevel simplification_level);

        /*
            Generates and uploads the newly generated chunk mesh right away
        */
        void syncGenerateSyncUploadMesh(Chunk* chunk, RegionCuller& buffer, BitField3D::SimplificationLevel simplification_level);

        void setWorld(Terrain* world){this->world = world;}
};