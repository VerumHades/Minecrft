#ifndef MESH_HEADER
#define MESH_HEADER

#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>


class Mesh{
    private:
        std::vector<float> vertices;
        std::vector<uint32_t> indices;
        std::vector<int> format;

        std::unique_ptr<GLTexture> texture;
        bool formatSet = false;

    public:
        size_t vertexSize = 0;

        Mesh();

        void setVertexFormat(const std::vector<int>& format);
        void addQuadFaceGreedy(glm::vec3 vertices[4], glm::vec3 normals[4],float metadata[6], int clockwise, int width, int height);
        void addQuadFace(glm::vec3 vertices[4], glm::vec3 normals[4], int clockwise);

        std::vector<float>& getVertices();
        std::vector<uint32_t>& getIndices();
        std::vector<int>& getFormat();

        void setTexture(const char* filename) {texture = std::make_unique<GLTexture>(filename);};
};
#endif