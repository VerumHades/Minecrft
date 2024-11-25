#pragma once

#include <unordered_map>
#include <glm/glm.hpp>
#include <array>
#include <rendering/buffer.hpp>

class Model{
    private:
        size_t request_size = 3;
        size_t last_request;
        std::vector<float> draw_request_data;

        GLVertexArray vao;

        GLBuffer<float, GL_ARRAY_BUFFER> vertex_buffer;
        GLBuffer<float, GL_ARRAY_BUFFER> instance_buffer;
        GLBuffer<uint , GL_ELEMENT_ARRAY_BUFFER> index_buffer;
    
    public:
        Model();
        /*
            Adds a position to where an instance of the model will be drawn
        */
        void requestDraw(glm::vec3 position);
        void drawAllRequests();
};