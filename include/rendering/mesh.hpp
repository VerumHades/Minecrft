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

        bool formatSet = false;

    public:
        size_t vertexSize = 0;

        Mesh();

        void setVertexFormat(const std::vector<int>& format);
        void addQuadFaceGreedy(glm::vec3 vertices[4], glm::vec3 normals,float metadata[6], int clockwise, int width, int height);
        void addQuadFace(glm::vec3 vertices[4], glm::vec3 normals, int clockwise);

        const std::vector<float>& getVertices();
        const std::vector<uint32_t>& getIndices();
        const std::vector<int>& getFormat();
};
#endif