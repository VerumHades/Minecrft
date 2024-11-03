#ifndef BUFFER_H
#define BUFFER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <cstring>
#include <queue>
#include <glm/glm.hpp>
#include <unordered_map>
#include <list>

#include <rendering/mesh.hpp>
#include <rendering/allocator.hpp>

#include <chrono>

void checkGLError(const char *file, int line);
#define CHECK_GL_ERROR() checkGLError(__FILE__, __LINE__)
class Mesh;

class GLBuffer{
    private:
        uint32_t data;
        uint32_t index;
        uint32_t vao;

        size_t vertexCount = 0;
        size_t indiciesCount = 0;

        bool dataLoaded = false;
    public:
        GLBuffer();
        ~GLBuffer();
        void loadMesh(Mesh& mesh);
        void draw();
        void drawInstances(int count);

        uint32_t getID(){ return data; } 
};

class GLStripBuffer{
    private:
        uint32_t data;
        uint32_t vao;

        size_t buffer_size = 0;
        size_t data_size = 0;

        VertexFormat format;
        void applyFormat();

    public:
        GLStripBuffer(VertexFormat format);
        ~GLStripBuffer();

        void setFormat(VertexFormat format);

        // Appends data and returns its starting point
        size_t appendData(const float* data, size_t size);
        // Inserts data at a given location
        bool insertData(size_t start, const float* data, size_t size);
        bool resize(size_t newSize);

        void draw();
};

class GLDoubleBuffer{
    private:
        GLBuffer buffers[2];
        int current = 0;
    public:
        void swap();
        GLBuffer& getBuffer();
        GLBuffer& getBackBuffer();
};

#include <vec_hash.hpp>

struct DrawElementsIndirectCommand {
    GLuint  count;      // Number of indices for the current draw call.
    GLuint  instanceCount; // Number of instances to render.
    GLuint  firstIndex; // Offset into the element array buffer.
    GLuint  baseVertex; // Base vertex for index calculations.
    GLuint  baseInstance; // Base instance for instanced rendering.
};

class MultiChunkBuffer{
    private:
        uint32_t maxDrawCalls = 0; // Calculated in the constructor
        uint32_t maxVertices = 0;
        uint32_t maxIndices = 0;

        VertexFormat vertexFormat;

        uint32_t indirectBuffer;
        uint32_t vertexBuffer;
        uint32_t indexBuffer;
        uint32_t vao;

        Allocator vertexAllocator;
        Allocator indexAllocator;
    
        struct LoadedChunk{ 
            size_t vertexData;
            size_t indexData;

            size_t firstIndex;
            size_t count;
            size_t baseVertex;

            size_t vertexDataSize;
            size_t indexDataSize;
        };

        //void setDrawCall(size_t index, uint32_t firstIndex, uint32_t count, uint32_t baseVertex);

        size_t drawCallBufferSize = 0;
        size_t drawCallCount = 0;
        size_t bufferOffset = 0;
        DrawElementsIndirectCommand* persistentMappedBuffer;
        
        std::unordered_map<glm::ivec3, LoadedChunk, IVec3Hash, IVec3Equal> loadedChunks;

    public:
        ~MultiChunkBuffer();
        
        void initialize(uint32_t renderDistance);

        bool addChunkMesh(Mesh& mesh, const glm::ivec3& pos);
        bool swapChunkMesh(Mesh& mesh, const glm::ivec3& pos);
        void unloadChunkMesh(const glm::ivec3& pos);
        void unloadFarawayChunks(const glm::ivec3& from, float treshold);
        void clear();
        bool isChunkLoaded(const glm::ivec3& pos){
            return loadedChunks.count(pos) != 0;
        }

        void updateDrawCalls(std::vector<DrawElementsIndirectCommand>& commands);
        DrawElementsIndirectCommand getCommandFor(const glm::ivec3& pos);
        void draw();

        Allocator& getVertexAllocator() {return vertexAllocator;};
        Allocator& getIndexAllocator() {return indexAllocator;};
};

#include <rendering/mesh.hpp>

#endif