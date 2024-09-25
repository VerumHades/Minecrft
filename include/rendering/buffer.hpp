#ifndef BUFFER_H
#define BUFFER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <queue>
#include <glm/glm.hpp>
#include <rendering/allocator.hpp>

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

class MultiChunkBuffer{
    private:
        uint32_t maxDrawCalls = 0; // Calculated in the constructor
        uint32_t maxVertices = 0;
        uint32_t maxIndices = 0;

        std::queue<uint32_t> freeDrawCallIndices;

        uint32_t indirectBuffer;
        uint32_t vertexBuffer;
        uint32_t indexBuffer;
        uint32_t vao;

        std::unique_ptr<Allocator<GLfloat>> vertexAllocator;
        std::unique_ptr<Allocator<GLuint>> indexAllocator;
    
        struct DrawElementsIndirectCommand {
            GLuint  count;      // Number of indices for the current draw call.
            GLuint  instanceCount; // Number of instances to render.
            GLuint  firstIndex; // Offset into the element array buffer.
            GLuint  baseVertex; // Base vertex for index calculations.
            GLuint  baseInstance; // Base instance for instanced rendering.
        };

        struct DrawCall{ 
            uint32_t firstIndex;
            uint32_t count;
            uint32_t index;
        };

        struct LoadedChunk{ 
            GLfloat* vertexData;
            GLuint* indexData;

            DrawCall drawCall;
        };

        DrawElementsIndirectCommand* drawCallBuffer = nullptr;
        DrawCall addDrawCall(uint32_t firstIndex, uint32_t count);

        std::unordered_map<glm::vec3, LoadedChunk, Vec3Hash, Vec3Equal> loadedChunks;

    public:
        MultiChunkBuffer(uint32_t maxDrawCalls);
        ~MultiChunkBuffer();

        void addChunkMesh(Mesh& mesh, const glm::vec3& pos);
        void draw();
};

#include <rendering/mesh.hpp>

#endif