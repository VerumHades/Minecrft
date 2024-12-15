#ifndef MESH_HEADER
#define MESH_HEADER

#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <array>
#include <cstring>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <general.hpp>

#include <rendering/opengl/buffer.hpp>

class Mesh;

class LoadedMesh{
    private:
        GLVertexArray vao;
        GLBuffer<float, GL_ARRAY_BUFFER> vertex_buffer;
        GLBuffer<uint , GL_ELEMENT_ARRAY_BUFFER> index_buffer;
        
        GLBuffer<float, GL_ARRAY_BUFFER> instance_buffer;

        LoadedMesh(Mesh& mesh, std::vector<GLVertexValueType> instance_types);
        friend class Mesh;

    public:
        const GLVertexArray& getVAO() {return vao;};
};

class Mesh{
    private:
        std::vector<float> vertices;
        std::vector<uint> indices;
        GLVertexFormat vertex_format;

    public:
        Mesh(GLVertexFormat vertex_format): vertex_format(vertex_format){}

        void addQuadFaceGreedy(glm::vec3 vertices_[4], int normal, float vertexOcclusion[4], float textureIndex, int clockwise, int width, int height);
        void addQuadFace(std::array<glm::vec3, 4> vertices, glm::vec3 normal, bool clockwise, std::vector<float> metadata = {});

        std::vector<float>& getVertices() {return this->vertices;}
        std::vector<uint>& getIndices() {return this->indices;}
        friend class LoadedMesh;

        LoadedMesh load(std::vector<GLVertexValueType> instance_types = {}) { return LoadedMesh(*this, instance_types); };
};
#endif