#pragma once

#include <rendering/model.hpp>
#include <rendering/mesh.hpp>

class CubeModel: public Model{
    public:
        CubeModel(std::string texture_path){
            Mesh mesh;

            std::array<glm::vec3,8> vertices = {
                glm::vec3(0.0f,1.0f,0.0f),
                glm::vec3(1.0f,1.0f,0.0f),
                glm::vec3(1.0f,0.0f,0.0f),
                glm::vec3(0.0f,0.0f,0.0f),
                glm::vec3(0.0f,1.0f,1.0f),
                glm::vec3(1.0f,1.0f,1.0f),
                glm::vec3(1.0f,0.0f,1.0f),
                glm::vec3(0.0f,0.0f,1.0f)
            };

            mesh.addQuadFace({vertices[0],vertices[1],vertices[2],vertices[3]}, {0,0,-1}, true ); // Front face
            mesh.addQuadFace({vertices[4],vertices[5],vertices[6],vertices[7]}, {0,0, 1}, false); // Back face

            mesh.addQuadFace({vertices[0],vertices[4],vertices[7],vertices[3]}, {-1,0,0}, false); // Left face
            mesh.addQuadFace({vertices[1],vertices[5],vertices[6],vertices[2]}, { 1,0,0}, true ); // Right face

            mesh.addQuadFace({vertices[0],vertices[4],vertices[5],vertices[1]}, {0,1,0}, true ); // Top face
            mesh.addQuadFace({vertices[3],vertices[7],vertices[6],vertices[2]}, {0,-1,0}, false); // Bottom face

            vertex_buffer.initialize(mesh.getVertices().size());
            vertex_buffer.insert(0, mesh.getVertices().size(), mesh.getVertices().data());

            index_buffer.initialize(mesh.getIndices().size());
            index_buffer.insert(0, mesh.getIndices().size(), mesh.getIndices().data());

            texture = std::make_shared<GLTexture>(texture_path.c_str());
        }
};