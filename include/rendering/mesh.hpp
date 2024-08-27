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
        std::vector<unsigned int> indices;
        std::vector<int> format;

        bool formatSet = false;

    public:
        size_t vertexSize = 0;

        Mesh();

        void setVertexFormat(const std::vector<int>& format);
        void addQuadFace(glm::vec3 vertices[4], glm::vec3 normals, std::vector<float> metadata, int clockwise, int width, int height);

        const std::vector<float>& getVertices();
        const std::vector<unsigned int>& getIndices();
        const std::vector<int>& getFormat();
};
#endif