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
                0,3,1, 3,2,1, // Front face
                4,5,7, 5,6,7, // Back face

                0,1,4, 1,5,4, // Top face
                3,7,2, 2,7,6, // Bottom face

                1,2,6, 1,6,5, // Right face,
                0,7,3, 0,4,7, // Left face
            };
            index_buffer.initialize(indices.size());
            index_buffer.insert(0, indices.size(), indices.data());

            setupBufferFormat({VEC3});
        }
};