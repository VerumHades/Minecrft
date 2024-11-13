#pragma once

#include <vec_hash.hpp>

#include <rendering/mesh.hpp>
#include <rendering/allocator.hpp>
#include <rendering/buffer.hpp>
#include <rendering/culling.hpp>

#include <sstream>    
#include <iomanip>    
#include <string>    
#include <bitset>

struct TransformHash;
struct TransformEqual;
class ChunkMeshRegistry;

struct DrawElementsIndirectCommand {
    GLuint  count;      // Number of indices for the current draw call.
    GLuint  instanceCount; // Number of instances to render.
    GLuint  firstIndex; // Offset into the element array buffer.
    GLuint  baseVertex; // Base vertex for index calculations.
    GLuint  baseInstance; // Base instance for instanced rendering.
};

class MeshRegion{
    private:
        struct Transform{
            /*
                Position within its level, actual position is 2^level * position
            */
            glm::ivec3 position = {0,0,0};
            /*
                Smallest is 1.
                The actuall size of a region based on its level is: 2^level
            */
            uint32_t level = 1;

        } transform;

        ChunkMeshRegistry& registry;
        MeshRegion(Transform& transform, ChunkMeshRegistry& registry): transform(transform), registry(registry) {}
        /*
            If the region has a single joined mesh
        */
        bool merged = false;
        /*
            If region is a part of a higher merged mesh
        */
        bool part_of_parent_mesh = false;
        /*
            Has no mesh
        */
        bool meshless = false;


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
            If merged breaks up the mesh back into subregions
        */
        bool split();


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
            Recalculates mesh indices to have no offset
        */
        void recalculateIndicies();

        /*
            Information drawable only if everything is merged, marks the respective positions and sizes of space allocated for the mesh
            Information will remain valid even for regions merged within parent regions but it will not be drawable
        */
        struct MeshInformation{ 
            size_t vertex_data_start = 0;
            size_t index_data_start  = 0;

            size_t vertex_data_size = 0;
            size_t index_data_size  = 0;

            // Information for the draw call
            size_t first_index = 0;
            size_t count = 0;
            size_t base_vertex  = 0;

            size_t index_offset = 0; // The last offset applied to the mesh when moved
        } mesh_information;

        /*
            Updates mesh information, will cause parent regions to split apart if they merged this one
        */
        bool updateMeshInformation(MeshInformation information);

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
        uint8_t child_states = 0x00;
        
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
        glm::ivec3 getParentRelativePosition();

        /*
            Returns the position of the regions parent
        */
        glm::ivec3 getParentPosition() {
            return {
                std::floor(static_cast<float>(transform.position.x) / 2.0f),
                std::floor(static_cast<float>(transform.position.y) / 2.0f),
                std::floor(static_cast<float>(transform.position.z) / 2.0f)
            };
        }

        friend struct TransformHash;
        friend struct TransformEqual;
        friend class ChunkMeshRegistry;
        /*
            Sets the state for a subregion in the 'child_states', coordinates are relative coordinates in the subregions level.

            return if the operation was successful.
        */
        bool setSubregionMeshState(uint32_t x, uint32_t y, uint32_t z, bool state);

        /*
            Sets the regions mesh state in the parent if possible
        */
        bool setStateInParent(bool value);
    public:
        
        
        /*
            If the regions mesh is contiguous, that means if its merged or that its level 1.

            This will be true for all drawable regions.
        */
        bool hasContiguousMesh() {return merged || transform.level == 1; }
        
        // Creates a draw command from current mesh information
        DrawElementsIndirectCommand generateDrawCommand();
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

class ChunkMeshRegistry{
    private:
        VertexFormat vertexFormat;

        uint32_t vao;

        Allocator vertexAllocator;
        Allocator indexAllocator;
        
        size_t drawCallBufferSize = 0;
        size_t drawCallCount = 0;

        size_t vertexBufferSize = 0;
        size_t indexBufferSize = 0;

        size_t maxVertexCount = 0;
        size_t maxIndexCount = 0;

        size_t maxDrawCalls = 0;

        std::unique_ptr<GLPersistentBuffer<DrawElementsIndirectCommand>> persistentDrawCallBuffer;
        std::unique_ptr<GLPersistentBuffer<float>> persistentVertexBuffer;
        std::unique_ptr<GLPersistentBuffer<uint32_t>> persistentIndexBuffer;

        // Highest region level, no regions higher than maxRegionLevel will be registered or created
        const static uint32_t maxRegionLevel = 5;
        
        // index 0 is level 1, precalculated sizes
        std::vector<uint32_t> actualRegionSizes;

        std::unordered_map<MeshRegion::Transform, MeshRegion, TransformHash, TransformEqual> regions;

        /*
            Allocates and copies the buffer into the respective buffers,

            returns the a tuple: 
                - bool => did the allocation succeed
                - size_t => vertex data offset in the vertex buffer
                - size_t => index data offset in the index buffer 
            
        */
        std::tuple<bool,size_t,size_t> allocateAndUploadMesh(Mesh* mesh);

        /*
            Checks the region againist the frustum. Does an octree search among its children.
            Ignores region that arent drawn.
            Writes their calls directly into the draw call buffer begining at the draw call index.
        */
        void processRegionForDrawing(Frustum& frustum, MeshRegion* region, int& drawCallIndex);

    public:
        ~ChunkMeshRegistry();
        
        void initialize(uint32_t renderDistance);

        void clear();
        bool isChunkLoaded(const glm::ivec3& pos){
            return getRegion({pos,1}) != nullptr;
        }

        /*
            Updates draw calls to the ones visible in the frustum.

            Takes the camera position in world coordinates
        */
        void updateDrawCalls(glm::ivec3 camera_position, Frustum& frustum);

        /*
            Uploads the mesh as a level 1 region. (All parents are automatically created if they dont already exist)

            Empty meshes will be ingnored. But registered as empty regions.
        */
        bool addMesh(Mesh* mesh, const glm::ivec3& pos);

        /*
            Updates a mesh, splits regions if old mesh is a part of them
        */
        bool updateMesh(Mesh* mesh, const glm::ivec3& pos);

        /*
            Creates a region and all its parents if they dont already exist.

            returns a `nullptr` on failure
        */
        MeshRegion* createRegion(MeshRegion::Transform transform);

        MeshRegion* getRegion(MeshRegion::Transform transform) {
            if(!regions.contains(transform)) return nullptr;
            return &regions.at(transform);
        }

        /*
            Return the real size of a region based on its level
        */
        int getRegionSizeForLevel(uint32_t level) {
            if(level < 1 || level > maxRegionLevel) return 0;
            return actualRegionSizes[level - 1];
        }

        float* getVertexBuffer(){ return persistentVertexBuffer->data(); }
        uint32_t* getIndexBuffer() { return persistentIndexBuffer->data(); }

        // The maximal number of floats the buffer can store
        size_t getVertexBufferSize() { return maxVertexCount; }
        // The maximal number of uint32_ts the buffer can store
        size_t getIndexBufferSize() { return maxIndexCount; }

        void setDrawCallCount(int value){drawCallCount = value;}

        DrawElementsIndirectCommand getCommandFor(const glm::ivec3& pos);
        void draw();

        Allocator& getVertexAllocator() {return vertexAllocator;};
        Allocator& getIndexAllocator() {return indexAllocator;};
        VertexFormat& getVertexFormat() {return vertexFormat;}
};

#include <rendering/mesh.hpp>