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

template <typename T>
class GLPersistentBuffer{
    private:
        uint32_t buffer_id;
        uint32_t type;
        size_t size;
        T* _data;
    
    public:
        GLPersistentBuffer(size_t size, uint32_t type): type(type), size(size){
            glGenBuffers(1, &buffer_id);
            glBindBuffer(type, buffer_id);
            glBufferStorage(type, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

            _data = static_cast<T*>(glMapBufferRange(type, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT));

            if(!_data) {
                throw std::runtime_error("Failed to map persistent buffer.");
            }
        }

        ~GLPersistentBuffer(){
            glDeleteBuffers(1, &buffer_id);
        }

        uint32_t getID() {return buffer_id;}
        T* data() {return this->_data;};

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