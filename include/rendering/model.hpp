#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>  

#include <array>
#include <rendering/buffer.hpp>
#include <rendering/texture.hpp>

#include <synchronization.hpp>

class Model{ 
    private:
        size_t request_size = 4 * 4;
        
        PassTroughBuffer<float> request_buffer;
        std::atomic<bool> pending_update = false;
        std::atomic<bool> upload_data = false;

        GLVertexArray vao;
        GLDoubleBuffer<float, GL_ARRAY_BUFFER> instance_buffer;
        
    protected:
        std::shared_ptr<GLTexture> texture = nullptr;

        GLBuffer<float, GL_ARRAY_BUFFER> vertex_buffer;
        GLBuffer<uint , GL_ELEMENT_ARRAY_BUFFER> index_buffer;

        void setupBufferFormat(std::vector<GLSlotBinding> bindings);

    public:
        Model();
        /*
            Adds a position to where an instance of the model will be drawn

            Rotation is in degrees
        */
        void resetRequests();
        void requestDraw(glm::vec3 position, glm::vec3 scale = {1,1,1}, glm::vec3 rotation = {0,0,0}, glm::vec3 rotation_center_offset = {0,0,0});
        void passRequests(); // Sends requests for actual rendering
        
        void drawAllRequests();
        void updateRequestBuffer();
};