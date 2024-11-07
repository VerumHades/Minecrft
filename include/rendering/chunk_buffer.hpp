#pragma once

#include <vec_hash.hpp>

#include <rendering/mesh.hpp>
#include <rendering/allocator.hpp>
#include <rendering/buffer.hpp>

#include <unordered_set>

struct TransformHash;
struct TransformEqual;
class MeshRegionManager;

class MeshRegion{
    private:
        struct Transform{
            /*
                Position within its level, actual position is 2^level * position
            */
            glm::ivec3 position;
            /*
                Smallest is 1.
                The actuall size of a region based on its level is: 2^level
            */
            uint32_t level;

        } transform;

        
        /*
            If the region has a single joined mesh
        */
        bool merged = false;
        /*
            If region is a part of a higher merged mesh
        */
        bool part_of_parent_mesh = false;

        /*
            If possible reorganizes all subregion meshes into one large mesh that can be drawn with a single draw call

            Fails if:
                - The region is already meshed
                - The region is a level 1 region (cannot be merged, because it has no subregions)
                - Not all subregions are meshed
                - Not all subregions meshes are (one mesh for subregion) or can be made contiquous (by merging)
                - Not all subregions exist
                - There is not enough space to allocate for the new mesh

            Will try to merge subregions
        */
        bool merge();

        /*
            Moves the mesh information in memory to a new position, only possible on level 1 regions or merged regions.
            
            Does no checks for intersection with existing meshes.
            Does not respect allocators.
            Does not clean the old data.

            Will fail if the new position is outside of the buffer.

            index_offset = an offset to apply to all indices
        */
        bool moveMesh(size_t new_vertex_position, size_t new_index_position, size_t index_offset = 0);

        /*
            Information valid only if everything is merged, marks the respective positions and sizes of space allocated for the agregate mesh
        */
        struct MeshInformation{ 
            size_t vertex_data_start;
            size_t index_data_start;

            size_t vertex_data_size;
            size_t index_data_size;

            // Information for the draw call
            size_t first_index;
            size_t count;
            size_t base_vertex;
        } mesh_information;

        /*
            Returns an index from subregions relative position
        */
        uint32_t getSubregionIndexFromPosition(uint32_t x, uint32_t y, uint32_t z) {
            return x + y * 2 + z * 4;
        }

        /*
            An 8 bit value to represent if subregions of this region have complete meshes

            Level 1 regions are complete when they have their corresponding mesh
        */
        uint8_t child_states;
        
        /*
            Returns a pointer to the regions parent, if the region has no parents return nullptr
        */
        MeshRegion* getParentRegion();
        /*
            Returns a pointer to the subregion in relative coordinates to the parent

            return nullptr if the region is at level 1 (there are no levels under 1).
        */
        MeshRegion* getSubregion(uint32_t x, uint32_t y, uint32_t z);
        /*
            Returns a position relative to the parent

            will return {0,0,0} if no parent is present
        */
        std::tuple<uint32_t,uint32_t,uint32_t> getParentRelativePosition();

        // Creates a draw command from current mesh information
        DrawElementsIndirectCommand generateDrawCommand();

        MeshRegionManager& manager;

        friend struct TransformHash;
        friend struct TransformEqual;
        friend class MeshRegionManager;

    public:
        /*
            Sets the state for a subregion in the 'child_states', coordinates are relative coordinates in the subregions level.

            return if the operation was successful.
        */
        bool setSubregionMeshState(uint32_t x, uint32_t y, uint32_t z, bool state);
        
        /*
            If the regions mesh is contiguous, that means if its merged or that its level 1.
        */
        bool hasContiguousMesh() {return merged || transform.level <= 1; }
};

struct TransformHash{
    std::size_t operator()(const MeshRegion::Transform& transform) const {
        std::size_t h1 = std::hash<float>{}(transform.position.x);
        std::size_t h2 = std::hash<float>{}(transform.position.y);
        std::size_t h3 = std::hash<float>{}(transform.position.z);
        std::size_t h4 = std::hash<int>{}(transform.level);

        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

struct TransformEqual {
    bool operator()(const MeshRegion::Transform& a, const MeshRegion::Transform& b) const {
        return a.position == b.position && a.level == b.level;
    }
};

struct DrawElementsIndirectCommand {
    GLuint  count;      // Number of indices for the current draw call.
    GLuint  instanceCount; // Number of instances to render.
    GLuint  firstIndex; // Offset into the element array buffer.
    GLuint  baseVertex; // Base vertex for index calculations.
    GLuint  baseInstance; // Base instance for instanced rendering.
};

class MeshRegionManager{
    private:
        VertexFormat vertexFormat;

        uint32_t vao;
        uint32_t indirectBuffer;
        uint32_t vertexBuffer;
        uint32_t indexBuffer;

        Allocator vertexAllocator;
        Allocator indexAllocator;
        
        size_t drawCallBufferSize = 0;
        size_t drawCallCount = 0;
        size_t bufferOffset = 0;

        size_t vertexBufferSize = 0;
        size_t indexBufferSize = 0;

        size_t maxVertexCount = 0;
        size_t maxIndexCount = 0;

        size_t maxDrawCalls = 0;

        DrawElementsIndirectCommand* persistentDrawCallBuffer;
        float* persistentVertexBuffer;
        uint32_t* persistentIndexBuffer;
        
        struct LoadedChunk{ 
            size_t vertexData;
            size_t indexData;

            size_t firstIndex;
            size_t count;
            size_t baseVertex;

            size_t vertexDataSize;
            size_t indexDataSize;
        };

        std::unordered_map<glm::ivec3, LoadedChunk, IVec3Hash, IVec3Equal> loadedChunks;


        std::unordered_map<MeshRegion::Transform, MeshRegion, TransformHash, TransformEqual> regions;
        /*
            Allocates and copies the buffer into the respective buffers,

            returns the a tuple: 
                - bool => did the allocation succeed
                - size_t => vertex data offset in the vertex buffer
                - size_t => index data offset in the index buffer 
            
        */
        std::tuple<bool,size_t,size_t> allocateAndUploadMesh(Mesh& mesh);

    public:
        ~MeshRegionManager();
        
        void initialize(uint32_t renderDistance);

        /*
            Creates and uploads a mesh region and registers it to the position 
        */
        //bool addMeshRegion(std::vector<std::unique_ptr<Mesh>>& meshes,const glm::ivec3& position);

        bool addChunkMesh(Mesh& mesh, const glm::ivec3& pos);
        bool swapChunkMesh(Mesh& mesh, const glm::ivec3& pos);
        void unloadChunkMesh(const glm::ivec3& pos);
        void unloadFarawayChunks(const glm::ivec3& from, float treshold);
        void clear();
        bool isChunkLoaded(const glm::ivec3& pos){
            return loadedChunks.count(pos) != 0;
        }

        MeshRegion* getRegion(MeshRegion::Transform transform) {
            if(!regions.contains(transform)) return nullptr;
            return &regions[transform];
        }

        float* getVertexBuffer(){ return persistentVertexBuffer; }
        uint32_t* getIndexBuffer() { return persistentIndexBuffer; }

        // The maximal number of floats the buffer can store
        size_t getVertexBufferSize() { return maxVertexCount; }
        // The maximal number of uint32_ts the buffer can store
        size_t getIndexBufferSize() { return maxIndexCount; }

        DrawElementsIndirectCommand* getCommandBuffer() {return persistentDrawCallBuffer;};
        void setDrawCallCount(int value){drawCallCount = value;}

        DrawElementsIndirectCommand getCommandFor(const glm::ivec3& pos);
        void draw();

        Allocator& getVertexAllocator() {return vertexAllocator;};
        Allocator& getIndexAllocator() {return indexAllocator;};
        VertexFormat& getVertexFormat() {return vertexFormat;}
};

#include <rendering/mesh.hpp>