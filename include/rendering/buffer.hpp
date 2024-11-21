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

class GLBufferLegacy{
    private:
        uint data;
        uint index;
        uint vao;

        size_t vertexCount = 0;
        size_t indiciesCount = 0;

        bool dataLoaded = false;
    public:
        GLBufferLegacy();
        ~GLBufferLegacy();
        void loadMesh(Mesh& mesh);
        void draw();
        void drawInstances(int count);

        uint getID(){ return data; } 
};

template <typename T, int type>
class GLBuffer{
    private:
        uint opengl_buffer_id;
        
        size_t buffer_size = 0;

        bool initialized = false;
    public:
        GLBuffer(){
            glGenBuffers(1, &opengl_buffer_id);
        }
        ~GLBuffer(){
            glDeleteBuffers(1, &opengl_buffer_id);
        }

        /*
            Creates the actual buffer of some size
        */
        void initialize(size_t size){
            if(size == 0) return;

            bind();
            glBufferData(type, size * sizeof(T), nullptr, GL_DYNAMIC_DRAW);
            buffer_size = size;

            initialize = true;
        }

        /*
            Inserts data into a buffer, needs to be initialized
        */
        bool insert(size_t at, size_t size, T* data){
            if(!initialized) throw std::runtime_error("Inserting data into an uninitialized buffer.");

            if(at + size >= buffer_size) return false; // Dont overflow

            bind();
            glBufferSubData(type, at * sizeof(T), size * sizeof(T), data);

            return true;
        }
        void bind(){
            glBindBuffer(type, opengl_buffer_id);
        }
};


template  <typename T, int type>
class GLAllocatedBuffer{
    private:
        Allocator allocator;
        GLBuffer<T,type> buffer;

        bool initialized = false;

    public:
        /*
            Creates the actual buffer of some size
        */
        void initialize(size_t size){
            buffer.initialize(size);
            allocator.initialize(size);
        }

        /*
            Allocates space in the buffer and returns the position if possible

            returns [success, position]

            success -> Whether the size was allocated (can fail because of insuficient size)
        */
        std::tuple<bool, size_t> insert(T* data, size_t size){
            auto [success, position] = allocator.allocate(size);

            if(!success) return {false};

            buffer.insert(position, size, data);

            return {true, position};
        }

        /*
            Allocates space in the buffer if its not enough, otherwise updates the data and returns the new position if possible

            returns [success, position]

            success -> Whether the size was allocated (can fail because of insuficient size)
        */
        std::tuple<bool, size_t> update(size_t at, size_t size, T* data){
            size_t block_size = allocator.getTakenBlockSize(at);
            if(block_size == 0) return {false}; // Invalid at

            if(block_size >= size){ // Space is sufficient
                buffer.insert(at, size, data);
                return {true, at};
            }

            allocator.free(at); //  Free the old allocated space
            return insert(at, size, data); // Allocate new space
        }

        /*
            Frees an allocated position for usage
        */
        void free(size_t position){
            allocator.free(position);
        }
};

/*
    A buffer of constant size but not persitently mapped
*/
template <typename T, int type>
class GLCachedDoubleBuffer{
    private:
        GLBuffer<T,type> buffer;

        size_t size_total = 0;
        size_t current_offset = 0;


        T* cache;
        size_t cache_size = 0;

        size_t last_cache_size = 0;

    public:
        size_t getCurrentOffset(){return current_offset;}

        /*
            Initialize with the size of a single portion, total size is double (double buffering)
        */
        GLCachedDoubleBuffer(size_t size): size_total(size * 2){
            cache = new T[size];

            buffer.initialize(size_total);
        }
        ~GLCachedDoubleBuffer(){
            delete cache;
        }

        /*
            Uploads cached data to the GPU and swaps
        */
        void flush(){
            current_offset = (current_offset + size_total / 2) % size_total;

            buffer.insert(current_offset, cache_size, cache);

            cache_size = 0;
        }

        void bind(){
            buffer.bind();
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
        GLBufferLegacy buffers[2];
        int current = 0;
    public:
        void swap();
        GLBufferLegacy& getBuffer();
        GLBufferLegacy& getBackBuffer();
};

#endif