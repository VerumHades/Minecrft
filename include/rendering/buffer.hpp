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

#include <chrono>

void checkGLError(const char *file, int line);
#define CHECK_GL_ERROR() checkGLError(__FILE__, __LINE__)

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

/*class GLStripBuffer{
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
        //bool resize(size_t newSize);

        void draw();
};*/

class GLDoubleBuffer{
    private:
        GLBuffer buffers[2];
        int current = 0;
    public:
        void swap();
        GLBuffer& getBuffer();
        GLBuffer& getBackBuffer();
};

#endif