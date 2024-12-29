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

#include <rendering/allocator.hpp>
#include <coherency.hpp>
#include <general.hpp>

#include <chrono>

template <typename T>
class GLBinding{
    private:
        T& bound_object;
    
    public:
        explicit GLBinding(T& bound_object): bound_object(bound_object) {
            bound_object.bind();
        }
        // Release the resource (unlock the mutex) in the destructor
        ~GLBinding() {
            bound_object.unbind();
        }
};

void checkGLError(const char *file, int line);
#define CHECK_GL_ERROR() checkGLError(__FILE__, __LINE__)

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

        GLBuffer(std::vector<T> data): GLBuffer(){
            initialize(data.size());
            insert(0,data.size(), data.data());
        }

        /*
            Creates the actual buffer of some size
        */
        virtual void initialize(size_t size, T* data = nullptr){
            if(size == 0) return;

            bind();
            glBufferData(type, size * sizeof(T), data, GL_DYNAMIC_DRAW);
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

        /*
            Writes the buffers contents into the destination
        */
        void get(T* destination, size_t size, size_t offset = 0){
            if(!initialized) throw std::runtime_error("Getting data from an uninitialized buffer.");

            glGetBufferSubData(type, offset * sizeof(T), size * sizeof(T), destination);
        }

        void bind(){
            glBindBuffer(type, opengl_buffer_id);
        }

        void bindBase(uint slot){
            glBindBufferBase(type, slot, opengl_buffer_id);
        }

        size_t size(){
            return buffer_size;
        }

        uint getID(){
            return opengl_buffer_id;
        }
};

template <typename T, int type>
class  GLDoubleBuffer{
    private:
        GLBuffer<T,type> buffers[2];
        int current = 0;
    public:
        void swap(){
            this->current = !this->current;
        }
        GLBuffer<T,type>& getBuffer(){
            return this->buffers[this->current];
        }
        GLBuffer<T,type>& getBackBuffer(){
            return this->buffers[!this->current];
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

            //std::cout << "Inserting at: " << position << " of actual size: " << aligned(size) * alignment << " for:" << size << std::endl;

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

            free(at); //  Free the old allocated space
            return insert(data, size); // Allocate new space
        }

        /*
            If at is not set inserts the mesh otherwise updates it
        */
        std::tuple<bool, size_t> insertOrUpdate(T* data, size_t size, size_t at = -1ULL){
            if(at != -1ULL) free(at);
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

class GLDrawCallBuffer{
    public:
        struct DrawCommand{
            GLuint count;         // Number of vertices to draw.
            GLuint instanceCount; // Number of instances to draw.
            GLuint first;         // Starting index in the vertex array.
            GLuint baseInstance;  // Instance ID of the first instance.
        };
        /*struct DrawCommand{ multiDrawElementsIndirect
            GLuint  count;      // Number of indices for the current draw call.
            GLuint  instanceCount; // Number of instances to render.
            GLuint  firstIndex; // Offset into the element array buffer.
            GLuint  baseVertex; // Base vertex for index calculations.
            GLuint  baseInstance; // Base instance for instanced rendering.
        };*/

    private:
        std::vector<DrawCommand> draw_commands;
        GLBuffer<DrawCommand, GL_DRAW_INDIRECT_BUFFER> buffer;

    public:
        void clear();
        void push(DrawCommand& command);
        void push(DrawCommand* commands, size_t size);
        void flush();
        void bind();

        // Count of draw commands
        size_t count() { return draw_commands.size(); };
};

/*
    CoherentList with a opengl buffer attached, flushes its contents into it
*/
template <typename T, int type>
class GLCoherentBuffer: public CoherentList<T> {
    private:
        GLBuffer<T, type> buffer;

    public:
        auto& getBuffer() { return buffer; };
        void flush(){
            if(CoherentList<T>::size() > buffer.size()) buffer.initialize(CoherentList<T>::size(), CoherentList<T>::data());
            else buffer.insert(0, CoherentList<T>::size(), CoherentList<T>::data());
        }
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

enum GLVertexValueType{
    FLOAT = 1,
    VEC2 = 2,
    VEC3 = 3,
    VEC4 = 4
};

class GLVertexFormat{
    private:
        std::vector<GLVertexValueType> bindings = {};
        uint totalSize = 0;
        bool per_instance = false;
    public:
        GLVertexFormat(std::initializer_list<GLVertexValueType> bindings, bool per_instance = false);
        void apply(uint& slot);
        uint getVertexSize(){return totalSize;}
};

/*
    A class to manage the vertex array object and its format
*/
class GLVertexArray{
    private:
        uint vao_id;

        struct BoundBuffer{
            GLBuffer<float, GL_ARRAY_BUFFER>* buffer_pointer;
            GLVertexFormat format;
        };

        std::vector<BoundBuffer> buffers;
    public:
        GLVertexArray(){
            glGenVertexArrays(1,  &vao_id);
        }
        ~GLVertexArray(){
            glDeleteVertexArrays(1,  &vao_id);
        }

        size_t attachBuffer(GLBuffer<float, GL_ARRAY_BUFFER>* buffer_pointer, GLVertexFormat format){
            buffers.push_back({buffer_pointer, format});

            update();
            
            return format.getVertexSize();
        }

        void attachBuffer(GLBuffer<uint, GL_ELEMENT_ARRAY_BUFFER>* buffer){
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

            for(auto& [buffer_pointer, format]: buffers){
                buffer_pointer->bind();
                format.apply(slot);
            }

            unbind();
        }
        void bind() const {
            glBindVertexArray(vao_id);
        }
        void unbind() const {
            glBindVertexArray(0);
        }
};
