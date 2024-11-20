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
        uint data;
        uint index;
        uint vao;

        size_t vertexCount = 0;
        size_t indiciesCount = 0;

        bool dataLoaded = false;
    public:
        GLBuffer();
        ~GLBuffer();
        void loadMesh(Mesh& mesh);
        void draw();
        void drawInstances(int count);

        uint getID(){ return data; } 
};

/*
    A buffer of constant size but not persitently mapped
*/
template <typename T, int type>
class GLConstantDoubleBuffer{
    private:
        int buffer_type = type;
        size_t size_total = 0;
        size_t current_offset = 0;

        uint buffer_id;

        T* cache;
        size_t cache_size = 0;

        size_t last_cache_size = 0;

    public:
        size_t getCurrentOffset(){return current_offset;}

        /*
            Initialize with the size of a single portion, total size is double (double buffering)
        */
        GLConstantDoubleBuffer(size_t size): size_total(size * 2){
            glGenBuffers(1, &this->buffer_id);

            cache = new T[size];

            glBindBuffer(type, buffer_id);
            glBufferData(type, size_total * sizeof(T), nullptr, GL_DYNAMIC_DRAW);

            CHECK_GL_ERROR();
            //std::cerr << "GLBuffer constructor called for object at " << this << std::endl;
        }
        ~GLConstantDoubleBuffer(){
            glDeleteBuffers(1 , &this->buffer_id);
            delete cache;
            //std::cerr << "GLBuffer destructor called for object at " << this << std::endl;
        }

        /*
            Uploads cached data to the GPU and swaps
        */
        void flush(){
            current_offset = (current_offset + size_total / 2) % size_total;

            glBindBuffer(type, buffer_id);
            glBufferSubData(type, current_offset * sizeof(T), cache_size * sizeof(T), cache);

            cache_size = 0;
        }

        void bind(){
            glBindBuffer(type, buffer_id);
        }

        /*
            Copies data into the temporary cache, size is in the number of elements T
        */
        bool appendData(T* data, size_t size){
            if(cache_size + size > size_total / 2) return false; // Down overflow
            
            std::memcpy(
                cache + cache_size,
                data,
                size * sizeof(T)
            );

            cache_size += size;
            return true;
        }
};

template <typename T>
class GLPersistentBuffer{
    private:
        uint buffer_id;
        uint type;
        size_t size;
        T* _data;
    
    public:
        GLPersistentBuffer(size_t size, uint type): type(type), size(size){
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

        uint getID() {return buffer_id;}
        T* data() {return this->_data;};

};


/*class GLStripBuffer{
    private:
        uint data;
        uint vao;

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