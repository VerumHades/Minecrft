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

#include <structure/allocator.hpp>
#include <coherency.hpp>
#include <general.hpp>

#include <chrono>
#include <unordered_set>

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

#ifdef GL_DIAGNOSTICS

template <typename T, int type>
class GLBuffer;

class GLBufferWrapper {
public:
    virtual ~GLBufferWrapper() = default; // Virtual destructor
    virtual size_t size() const = 0;
};

class GLBufferStatistics{
    private:
        std::unordered_set<GLBufferWrapper*> registered;

        GLBufferStatistics(){}
        static GLBufferStatistics& get(){
            static GLBufferStatistics stats;
            return stats;
        }

        template <typename T, int type>
        friend class GLBuffer;

    public:
        static size_t getMemoryUsage(){
            size_t total = 0;
            for(auto* buffer: get().registered)
                total += buffer->size();
            return total;
        }
};


template <typename T, int type>
class GLBuffer: public GLBufferWrapper {
#else

template <typename T, int type>
class GLBuffer {
#endif
    private:
        uint opengl_buffer_id = 0;
        
        size_t buffer_size = 0;

        bool initialized = false;
        bool invalidated = false;
    public:
        GLBuffer(){
            #ifdef GL_DIAGNOSTICS
            GLBufferStatistics::get().registered.emplace(this);
            #endif

            GL_CALL( glGenBuffers(1, &opengl_buffer_id));
        }
        ~GLBuffer(){
            if(!invalidated) cleanup();
        }

        GLBuffer(std::vector<T> data): GLBuffer(){
            initialize(data.size());
            insert(0,data.size(), data.data());
        }

        // Delete the copy constructor to make it non-copyable
        GLBuffer(const GLBuffer& other) = delete;

        // Delete the copy assignment operator to make it non-copyable
        GLBuffer& operator=(const GLBuffer& other) = delete;

        GLBuffer(GLBuffer&& other) noexcept {
            opengl_buffer_id = other.opengl_buffer_id;
            other.opengl_buffer_id = 0; // Null out the source pointer to indicate "moved-from"
            other.invalidated = true;
        }

        GLBuffer& operator=(GLBuffer&& other) noexcept {
            if (this != &other) {
                cleanup();
                invalidated = false;
                opengl_buffer_id = other.opengl_buffer_id;  // Steal the resource
                other.opengl_buffer_id = 0; // Null out the source pointer to indicate "moved-from"
                other.invalidated = true;
            }
            return *this;
        }


        /*
            Creates the actual buffer of some size
        */
        virtual void initialize(size_t size, T* data = nullptr){
            if(size == 0) return;

            bind();
            GL_CALL( glBufferData(type, size * sizeof(T), data, GL_DYNAMIC_DRAW));
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
            GL_CALL( glBufferSubData(type, at * sizeof(T), size * sizeof(T), data));

            return true;
        }

        /*
            Writes the buffers contents into the destination
        */
        void get(T* destination, size_t size, size_t offset = 0) const{
            if(!initialized) throw std::runtime_error("Getting data from an uninitialized buffer.");

            GL_CALL( glGetBufferSubData(type, offset * sizeof(T), size * sizeof(T), destination));
        }

        void bind() const{
            GL_CALL( glBindBuffer(type, opengl_buffer_id));
        }

        void bindBase(uint slot) const{
            GL_CALL( glBindBufferBase(type, slot, opengl_buffer_id));
        }

        #ifdef GL_DIAGNOSTICS
        size_t size() const override {
        #else
        size_t size() const {
        #endif
            return buffer_size;
        }

        uint getID() const{
            return opengl_buffer_id;
        }

        void cleanup(){
            GL_CALL( glDeleteBuffers(1, &opengl_buffer_id));
            invalidated = true;
            
            #ifdef GL_DIAGNOSTICS
            GLBufferStatistics::get().registered.erase(this);
            #endif
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

            return {success, position};
        }

    public:
        GLAllocatedBuffer(){}
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
            GL_CALL( glGenBuffers(1, &buffer_id));
            GL_CALL( glBindBuffer(type, buffer_id));
            GL_CALL( glBufferStorage(type, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT));

            _data = static_cast<T*>(glMapBufferRange(type, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT));

            if(!_data) {
                throw std::runtime_error("Failed to map persistent buffer.");
            }
        }

        ~GLPersistentBuffer(){
            GL_CALL( glDeleteBuffers(1, &buffer_id));
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
        std::vector<DrawCommand> draw_commands{};
        GLBuffer<DrawCommand, GL_DRAW_INDIRECT_BUFFER> buffer{};

    public:
        GLDrawCallBuffer(){}
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
        GLBuffer<T, type> buffer{};

    public:
        GLCoherentBuffer(){}
        auto& getBuffer() { return buffer; };
        void flush(){
            if(CoherentList<T>::size() > buffer.size()) buffer.initialize(CoherentList<T>::size(), CoherentList<T>::data());
            else buffer.insert(0, CoherentList<T>::size(), CoherentList<T>::data());
        }
};

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
        GLVertexFormat(){}
        void apply(uint& slot);
        uint getVertexSize(){return totalSize;}
};

/*
    A class to manage the vertex array object and its format
*/
class GLVertexArray{
    private:
        uint vao_id = 0;
        int index_counter = 100;

        struct BoundBuffer{
            GLBuffer<float, GL_ARRAY_BUFFER>* buffer_pointer = nullptr;
            GLVertexFormat format{};

            BoundBuffer(){}
            BoundBuffer(GLBuffer<float, GL_ARRAY_BUFFER>* buffer_pointer, const GLVertexFormat& format) :
                buffer_pointer(buffer_pointer), format(format){}
        };

        std::map<int,BoundBuffer> buffers{};
    public:
        GLVertexArray();
        ~GLVertexArray();

        GLVertexArray(const GLVertexArray& other) = delete;
        GLVertexArray& operator=(const GLVertexArray& other) = delete;

        GLVertexArray(GLVertexArray&& other) noexcept {
            vao_id = other.vao_id;
            other.vao_id = 0; // Null out the source pointer to indicate "moved-from"
        }

        GLVertexArray& operator=(GLVertexArray&& other) noexcept {
            if (this != &other) {
                GL_CALL( glDeleteVertexArrays(1,  &vao_id));
                vao_id = other.vao_id;  // Steal the resource
                other.vao_id = 0; // Null out the source pointer to indicate "moved-from"
            }
            return *this;
        }

        size_t attachBuffer(GLBuffer<float, GL_ARRAY_BUFFER>* buffer_pointer, GLVertexFormat format, int index = -1);

        void attachBuffer(GLBuffer<uint, GL_ELEMENT_ARRAY_BUFFER>* buffer);

        /*
            Updates buffers, bindings locations are based on how the buffers were attached sequentialy
        */
        void update();
        void bind() const;
        void unbind() const;
};
