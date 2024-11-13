#pragma once

#include <mutex>
#include <game/world/world.hpp>

class ChunkMeshGenerator{
    public:
        struct Face{
            int x;
            int y;
            int width;
            int height;
        };
    private:
        std::vector<Face> greedyMeshPlane(std::array<uint_t<64>, 64> rows, int size);
        std::unique_ptr<Mesh> generateChunkMesh(glm::ivec3 worldPosition, DynamicChunkContents* group);

        std::mutex meshLoadingMutex;
        
        struct MeshLoadingMember{
            glm::ivec3 position;
            std::unique_ptr<Mesh> mesh;
        };

        std::queue<MeshLoadingMember> meshLoadingQueue;

    public:
        void addToChunkMeshLoadingQueue(glm::ivec3 position, std::unique_ptr<Mesh> mesh);
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
};