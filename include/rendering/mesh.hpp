#ifndef MESH_HEADER
#define MESH_HEADER

#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <array>
#include <cstring>
#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <general.hpp>

#include <rendering/opengl/buffer.hpp>
#include <rendering/opengl/texture.hpp>

class Mesh;

class LoadedMesh{
    private:
        std::array<GLVertexArray,2> vaos{};
        GLBuffer<float, GL_ARRAY_BUFFER> vertex_buffer;
        GLBuffer<uint , GL_ELEMENT_ARRAY_BUFFER> index_buffer;
        std::shared_ptr<GLTexture2D> texture = nullptr;

        friend class Mesh;

    public:
        LoadedMesh(Mesh& mesh, std::shared_ptr<GLTexture2D> texture);

        const std::shared_ptr<GLTexture2D>& getTexture() {return texture;};
        size_t indicesTotal() {return index_buffer.size();};

        const auto& getVertexBuffer(){return vertex_buffer;};
        const auto& getIndexBuffer(){return index_buffer;};
        std::array<GLVertexArray,2>& getVAO() {return vaos;};
};

class Mesh{
    private:
        std::vector<float> vertices;
        std::vector<uint> indices;
        GLVertexFormat vertex_format;

        std::shared_ptr<GLTexture2D> texture = nullptr;

    public:
        Mesh(GLVertexFormat vertex_format, std::shared_ptr<GLTexture2D> texture = nullptr): vertex_format(vertex_format), texture(std::move(texture)){}

        void addQuadFaceGreedy(glm::vec3 vertices_[4], int normal, float vertexOcclusion[4], float textureIndex, int clockwise, int width, int height);
        void addQuadFace(std::array<glm::vec3, 4> vertices, glm::vec3 normal, bool clockwise,
            std::vector<float> metadata = {}, std::array<glm::vec2, 4> textureCoordinates = {glm::vec2{0, 0},{1, 0},{1, 1},{0, 1}});

        std::vector<float>& getVertices() {return this->vertices;}
        std::vector<uint>& getIndices() {return this->indices;}
        friend class LoadedMesh;

        void setTexture(std::shared_ptr<GLTexture2D> texture){this->texture = texture;}
        std::shared_ptr<GLTexture2D>& getTexture() {return texture;};

        std::unique_ptr<LoadedMesh> load() 
            { return std::make_unique<LoadedMesh>(*this, texture); };
};

#endif