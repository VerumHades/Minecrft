#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <cstring>
#include <queue>
#include <glm/glm.hpp>
#include <unordered_map>
#include <map>
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
        virtual void initialize(size_t size){
            if(size == 0) return;

            bind();
            glBufferData(type, size * sizeof(T), nullptr, GL_DYNAMIC_DRAW);
            buffer_size = size;

            initialized = true;
        }

        /*
            Inserts data into a buffer, needs to be initialized
        */
        virtual bool insert(size_t at, size_t size, T* data){
            if(!initialized) throw std::runtime_error("Inserting data into an uninitialized buffer.");

            if(at + size > buffer_size) return false; // Dont overflow

            bind();
            glBufferSubData(type, at * sizeof(T), size * sizeof(T), data);

            return true;
        }
        void bind(){
            glBindBuffer(type, opengl_buffer_id);
        }

        size_t size(){
            return buffer_size;
        }
};


template  <typename T, int type>
class GLAllocatedBuffer: public GLBuffer<T,type>{
    private:
        Allocator allocator;
        size_t alignment = 1;
        bool initialized = false;

        size_t aligned(size_t from){
            return std::ceil(static_cast<float>(from) / static_cast<float>(alignment));
        }

        std::tuple<bool, size_t> allocateAligned(size_t size){
            auto [success, position] = allocator.allocate(aligned(size));
            position *= alignment; // Aligned position

            return {success, position};
        }

    public:
        /*
            Creates the actual buffer of some size
        */
        void initialize(size_t size, size_t alignment = 1){
            GLBuffer<T,type>::initialize(size);
            this->alignment = alignment;
            allocator.reset(size / alignment);
        }
        /*
            Allocates space in the buffer and returns the position if possible

            returns [success, position]

            success -> Whether the size was allocated (can fail because of insuficient size)
        */
        std::tuple<bool, size_t> insert(T* data, size_t size){  
            auto [success, position] = allocateAligned(size);

            if(!success) return {false, 0};

            GLBuffer<T,type>::insert(position, size, data);

            return {true, position};
        }

        std::tuple<bool, size_t> allocateAhead(size_t size){
            return allocateAligned(size);
        }

        void insertDirect(size_t at, size_t size, T* data){
            GLBuffer<T,type>::insert(at, size, data);
        }

        /*
            Allocates space in the buffer if its not enough, otherwise updates the data and returns the new position if possible

            returns [success, position]

            success -> Whether the size was allocated (can fail because of insuficient size)
        */
        std::tuple<bool, size_t> update(size_t at, size_t size, T* data){
            size_t block_size = allocator.getTakenBlockSize(at);
            if(block_size == 0) return insert(data, size); // Invalid at

            if(block_size >= size){ // Space is sufficient
                GLBuffer<T,type>::insert(at, size, data);
                return {true, at};
            }

            allocator.free(at); //  Free the old allocated space
            return insert(data, size); // Allocate new space
        }

        /*
            If at is not set inserts the mesh otherwise updates it
        */
        std::tuple<bool, size_t> insertOrUpdate(T* data, size_t size, size_t at = -1ULL){
            if(at != -1ULL) allocator.free(at);
            return insert(data,size);
        }

        /*
            Frees an allocated position for usage
        */
        void free(size_t position){
            allocator.free(aligned(position));
        }

        const auto& getTakenBlocks() const {return allocator.getTakenBlocks();}
};

/*
    All added data is cached up to a set size and then flushed right before drawing
*/
template <typename T, int  type>
class GLLazyBuffer: public GLBuffer<T,type>{
    private:
        size_t buffer_write_position = 0;

        T* cache;
        size_t cache_max_size = 4096;
        size_t cache_write_position = 0;

    public:
        GLLazyBuffer(size_t size){
            cache = new T[size];

            GLBuffer<T,type>::initialize(size);
        }
        ~GLLazyBuffer(){
            delete cache;
        }
        /*
            Copies data into the temporary cache
        */
        bool append(T* data, size_t size){
            if(cache_write_position + size > cache_max_size){
                if(!flush()) return false; // Try to flush the cache, fail if the buffer is full
            }; // Down overflow
            
            std::memcpy(
                cache + cache_write_position,
                data,
                size * sizeof(T)
            );

            cache_write_position += size;
            return true;
        }

        /*
            Uploads cached data to the buffer
        */
        bool flush(){
            if(cache_write_position == 0) return true; // Nothing to flush

            bool output = GLBuffer<T,type>::insert(buffer_write_position, cache_write_position, cache);

            buffer_write_position += cache_write_position;
            cache_write_position = 0;

            return output;
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

class GLDoubleBuffer{
    private:
        GLBufferLegacy buffers[2];
        int current = 0;
    public:
        void swap();
        GLBufferLegacy& getBuffer();
        GLBufferLegacy& getBackBuffer();
};

/*
    A buffer that keeps all elements aligned in one contiguous block

template <typename T, int type>
class GLAlignedBuffer: public GLBuffer<T,type>{
    private:
        uint buffer_id;
        size_t buffer_size = 0;
        size_t pseudo_size = 0; // Last element

       
            Resizes the buffer, retains current data (if new size is smaller the excess data is lost)
       
        void resize(size_t new_size){
            if(new_size == buffer_size) return; // Already the set size

            uint new_buffer_id;
            glGenBuffers(1, &new_buffer_id);
            
            if(buffer_size > 0){
                size_t copy_size = std::min(new_size, buffer_size);

                glBindBuffer(GL_COPY_READ_BUFFER, buffer_id);
                glBindBuffer(GL_COPY_WRITE_BUFFER, new_buffer_id);

                glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, copy_size * sizeof(T));

                glBindBuffer(GL_COPY_READ_BUFFER, 0);
                glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
            }

            buffer_id = new_buffer_id;
            buffer_size = new_size;
        }

    public:
        GLVector(){
            glGenBuffers(1, &buffer_id);
        }
        ~GLVector(){
            glDeleteBuffers(1, &buffer_id);
        }

        void push_back(T element){

        }

        size_t size() { return buffer_size; }
};*/

enum GLSlotBinding{
    FLOAT = 1,
    VEC2 = 2,
    VEC3 = 3,
    VEC4 = 4
};

/*
    A class to manage the vertex array object and its format
*/
class GLVertexArray{
    private:
        uint vao_id;

        struct BoundBuffer{
            GLBuffer<float, GL_ARRAY_BUFFER>* buffer_pointer;
            std::vector<GLSlotBinding> bindings;
            size_t size;
            bool per_instance = false;
        };

        std::vector<BoundBuffer> buffers;
    public:
        GLVertexArray(){
            glGenVertexArrays(1,  &vao_id);
        }
        ~GLVertexArray(){
            glDeleteVertexArrays(1,  &vao_id);
        }

        size_t attachBuffer(GLBuffer<float, GL_ARRAY_BUFFER>* buffer_pointer, std::vector<GLSlotBinding> bindings, bool per_instance = false){
            size_t vertex_size = 0;
            for(auto& binding: bindings) vertex_size += binding;

            buffers.push_back({buffer_pointer, bindings, vertex_size, per_instance});

            update();
            
            return vertex_size;
        }
        void attachIndexBuffer(GLBuffer<uint, GL_ELEMENT_ARRAY_BUFFER>* buffer){
            bind();
            buffer->bind();
            unbind();
        }

        /*
            Updates buffers, bindings locations are based on how the buffers were attached sequentialy
        */
        void update(){
            bind();

            uint slot = 0;

            for(auto& [buffer_pointer, bindings, vertex_size, per_instance]: buffers){
                buffer_pointer->bind();

                size_t stride =  vertex_size * sizeof(float);
                size_t size_to_now = 0;

                for(auto& current_size: bindings){
                    /*if(current_size == MAT4){
                        glVertexAttribPointer(pos1, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(0));
                        glVertexAttribPointer(pos2, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(float) * 4));
                        glVertexAttribPointer(pos3, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(float) * 8));
                        glVertexAttribPointer(pos4, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(float) * 12));
                    }*/

                    uintptr_t pointer = size_to_now * sizeof(float);

                    glVertexAttribPointer(slot, (int) current_size, GL_FLOAT, GL_FALSE, (int)stride, (void*)pointer);
                    glEnableVertexAttribArray(slot);
                    if(per_instance) glVertexAttribDivisor(slot, 1);

                    size_to_now += current_size;
                    slot++;
                }
            }

            unbind();
        }
        void bind(){
            glBindVertexArray(vao_id);
        }
        void unbind(){
            glBindVertexArray(0);
        }
};
