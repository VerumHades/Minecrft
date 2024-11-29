#pragma once

#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>  

#include <array>
#include <rendering/buffer.hpp>
#include <rendering/texture.hpp>

class Model{ 
    private:
        size_t request_size = 4 * 4;
        size_t last_request = 0;
        std::vector<float> draw_request_data = {};

        GLVertexArray vao;
        GLBuffer<float, GL_ARRAY_BUFFER> instance_buffer;
        
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
        void requestDraw(glm::vec3 position, glm::vec3 scale = {1,1,1}, glm::vec3 rotation = {0,0,0});
        void drawAllRequests();
};