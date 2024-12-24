#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>  

#include <array>
#include <rendering/opengl/buffer.hpp>
#include <rendering/opengl/texture.hpp>

#include <synchronization.hpp>
#include <coherency.hpp>

class Model{ 
    private:
        size_t request_size = 4 * 4;
        
        std::atomic<bool> upload_data = false;
        std::atomic<int> selected = 0;

        std::array<GLVertexArray,2> vaos = {};
        std::array<GLBuffer<float, GL_ARRAY_BUFFER>,2> instance_buffers = {};
        std::array<std::vector<float>,2> request_buffers = {};

        GLBuffer<float, GL_ARRAY_BUFFER>& getBackInstanceBuffer()  { return instance_buffers[!selected]; }
        GLBuffer<float, GL_ARRAY_BUFFER>& getFrontInstanceBuffer() { return instance_buffers[selected]; }
        
    protected:
        std::shared_ptr<GLTexture2D> texture = nullptr;

        GLBuffer<float, GL_ARRAY_BUFFER> vertex_buffer;
        GLBuffer<uint , GL_ELEMENT_ARRAY_BUFFER> index_buffer;

    public:
        Model();
        /*
            Adds a position to where an instance of the model will be drawn

            Rotation is in degrees
        */
        void requestDraw(glm::vec3 position, glm::vec3 scale = {1,1,1}, glm::vec3 rotation = {0,0,0}, glm::vec3 rotation_center_offset = {0,0,0});

        void drawAllRequests();

        void swap() {
            selected = !selected;
            request_buffers[!selected].clear();
            upload_data = true;
        }
};