#pragma once

#include <rendering/model.hpp>

class CubeModel: public Model{
    public:
        CubeModel(){
            std::array<float,8 * 3> vertices = {
                0.0f,1.0f,0.0f,
                1.0f,1.0f,0.0f,
                1.0f,0.0f,0.0f,
                0.0f,0.0f,0.0f,
                0.0f,1.0f,1.0f,
                1.0f,1.0f,1.0f,
                1.0f,0.0f,1.0f,
                0.0f,0.0f,1.0f
            };
            vertex_buffer.initialize(vertices.size());
            vertex_buffer.insert(0, vertices.size(), vertices.data());

            std::array<uint, 6 * 6> indices = {
                1,3,0, 1,2,3, // Front face
                7,5,4, 7,6,5, // Back face

                4,1,0, 4,5,1, // Top face
                2,7,3, 6,7,2, // Bottom face

                6,2,1, 5,6,1, // Right face,
                3,7,0, 7,4,0 // Left face
            };
            index_buffer.initialize(indices.size());
            index_buffer.insert(0, indices.size(), indices.data());

            setupBufferFormat({VEC3});
        }
};