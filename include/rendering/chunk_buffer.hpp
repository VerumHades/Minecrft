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

    void print() const {
        std::cout << "DrawElementsIndirectCommand:" << std::endl;
        std::cout << "  count: " << count << std::endl;
        std::cout << "  instanceCount: " << instanceCount << std::endl;
        std::cout << "  firstIndex: " << firstIndex << std::endl;
        std::cout << "  baseVertex: " << baseVertex << std::endl;
        std::cout << "  baseInstance: " << baseInstance << std::endl;
    }
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
            uint level = 1;
        } transform;

        using DrawCommandList = std::vector<DrawElementsIndirectCommand>;
        // Commands to draw the region and all its subregions
        DrawCommandList draw_commands;

        ChunkMeshRegistry& registry;
        MeshRegion(Transform& transform, ChunkMeshRegistry& registry): transform(transform), registry(registry) {}
        
        /*
            Has no mesh
        */
        bool meshless = true;
        bool meshless_first = true;

        bool propagated = false; // If it has reported its existence to all parents already
        std::vector<std::tuple<Transform,size_t>> in_parent_draw_call_references; // References to all parent meshes

                
        // Creates a draw command from current mesh information
        DrawElementsIndirectCommand generateDrawCommand();
        /* 
            Information relevent for only level 1 meshes
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
        } mesh_information;

        /*
            Updates mesh information, will cause parent regions to split apart if they merged this one
        */
        bool updateMeshInformation(MeshInformation information);

        bool updatePropagatedDrawCalls();
        /*
            Establishes its own draw call in all the parent regions.

            Only for level 1 regions.
        */
        bool propagateDrawCall();

        void setMeshless(bool value);

        /*
            Returns an index from subregions relative position
        */
        uint getSubregionIndexFromPosition(uint x, uint y, uint z) {
            return x + y * 2 + z * 4;
        }
        
        /*
            Returns a pointer to the regions parent, if the region has no parents return nullptr
        */
        MeshRegion* getParentRegion();
        /*
            Returns a pointer to the subregion in relative coordinates to the parent

            return nullptr if the region is at level 1 (there are no levels under 1).
        */
        MeshRegion* getSubregion(uint x, uint y, uint z);
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

        uint vao;
        
        size_t drawCallCount = 0;

        size_t vertexBufferSize = 0;
        size_t indexBufferSize = 0;

        size_t maxVertexCount = 0;
        size_t maxIndexCount = 0;

        size_t maxDrawCalls = 0;

        std::unique_ptr<GLCachedDoubleBuffer<DrawElementsIndirectCommand, GL_DRAW_INDIRECT_BUFFER>> drawCallBuffer;

        GLAllocatedBuffer<float, GL_ARRAY_BUFFER        > vertexBuffer;
        GLAllocatedBuffer<uint , GL_ELEMENT_ARRAY_BUFFER> indexBuffer;

        // Highest region level, no regions higher than maxRegionLevel will be registered or created
        const static uint maxRegionLevel = 5;
        
        // index 0 is level 1, precalculated sizes
        std::vector<uint> actualRegionSizes;

        std::unordered_map<MeshRegion::Transform, MeshRegion, TransformHash, TransformEqual> regions;

        /*
            Allocates and copies the buffer into the respective buffers,

            returns the a tuple: 
                - bool => did the allocation succeed
                - size_t => vertex data offset in the vertex buffer
                - size_t => index data offset in the index buffer 
            
        */
        std::tuple<bool,size_t,size_t> allocateOrUpdateMesh(Mesh* mesh, size_t vertexPosition = -1ULL, size_t indexPosition = -1ULL);

        /*
            Checks the region againist the frustum. Does an octree search among its children.
            Ignores region that arent drawn.
            Writes their calls directly into the draw call buffer begining at the draw call index.
        */
        void processRegionForDrawing(Frustum& frustum, MeshRegion* region, size_t& draw_call_counter);

    public:
        ~ChunkMeshRegistry();
        
        void initialize(uint renderDistance);

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
        int getRegionSizeForLevel(uint level) {
            if(level < 1 || level > maxRegionLevel) return 0;
            return actualRegionSizes[level - 1];
        }

        // The maximal number of floats the buffer can store
        size_t getVertexBufferSize() { return maxVertexCount; }
        // The maximal number of uints the buffer can store
        size_t getIndexBufferSize() { return maxIndexCount; }

        void setDrawCallCount(int value){drawCallCount = value;}

        DrawElementsIndirectCommand getCommandFor(const glm::ivec3& pos);
        void draw();

        VertexFormat& getVertexFormat() {return vertexFormat;}
};

#include <rendering/mesh.hpp>