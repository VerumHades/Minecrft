#pragma once

#include <mutex>
#include <game/world/world.hpp>
#include <rendering/instanced_mesh.hpp>
#include <rendering/chunk_buffer.hpp>
#include <blockarray.hpp>
#include <glm/glm.hpp>

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

    private:
        std::vector<Face> greedyMeshPlane(BitPlane<64> rows, int size = 64);
        std::unique_ptr<InstancedMesh> generateChunkMesh(glm::ivec3 position, Chunk* group);

        BlockRegistry& blockRegistry;

        std::mutex meshLoadingMutex;
        
        struct MeshLoadingMember{
            glm::ivec3 position;
            std::unique_ptr<InstancedMesh> mesh;
        };

        std::queue<MeshLoadingMember> meshLoadingQueue;

        World* world = nullptr; // Points to the world relative to which you generate meshes, doesnt need to be set

        void addToChunkMeshLoadingQueue(glm::ivec3 position, std::unique_ptr<InstancedMesh> mesh);
        /*
            Creates separate planes from one plane with occlusion values
        */
        std::vector<OccludedPlane> calculatePlaneAmbientOcclusion(BitPlane<64>& source_plane, BitPlane<64>& occlusion_plane);

        /*
            Returns two plains separated by the occlusion at the offset and information whether they are empty
        */
        std::tuple<OccludedPlane, bool, OccludedPlane, bool> segregatePlane(
            OccludedPlane& source_plane,
            BitPlane<64>& occlusion_plane,
            std::array<bool,4> affects,
            glm::ivec2 lookup_offset
        );

        void proccessOccludedFaces(
            BitPlane<64>& source_plane,
            BitPlane<64>& occlusion_plane,
            
            InstancedMesh::FaceType face_type,
            InstancedMesh::Direction direction,
            BlockRegistry::BlockPrototype* type,
            InstancedMesh* mesh, 
            glm::vec3 world_position,
            int layer
        );
    
    public:
        ChunkMeshGenerator(BlockRegistry& blockRegistry): blockRegistry(blockRegistry) {}

        void loadMeshFromQueue(ChunkMeshRegistry&  buffer);

        /*
            Launches a thread if possible, the when the mesh is generated sends it to the worlds mesh loading queue,

            ISNT RESPONSIBLE FOR ACTUALLY UPLOADING THE MESH
        */
        void asyncGenerateAsyncUploadMesh(Chunk* chunk, ThreadPool& pool);

        /*
            When the mesh is generated sends it to the worlds mesh loading queue,

            ISNT RESPONSIBLE FOR ACTUALLY UPLOADING THE MESH
        */
        void syncGenerateAsyncUploadMesh(Chunk* chunk);

        /*
            Generates and uploads the newly generated chunk mesh right away
        */
        void syncGenerateSyncUploadMesh(Chunk* chunk, ChunkMeshRegistry& buffer);

        void setWorld(World* world){this->world = world;}
};