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
        int textureIndex = -1;

    public:
        size_t vertexSize = 0;

        Mesh();

        void setVertexFormat(const std::vector<int>& format);
        void addQuadFaceGreedy(glm::vec3 vertices_[4], glm::vec3 normals[4], float vertexOcclusion[4], float textureIndex, int clockwise, int width, int height);
        void addQuadFace(glm::vec3 vertices[4], glm::vec3 normals[4], int clockwise);
        void addFlatFace(int x, int y, int width, int height);

        std::vector<float>& getVertices() {return this->vertices;}
        std::vector<uint32_t>& getIndices() {return this->indices;}
        std::vector<int>& getFormat() {return this->format;}

        void setTextureIndex(int index){textureIndex = index;}
        int getTextureIndex(){return textureIndex;};
};
#endif