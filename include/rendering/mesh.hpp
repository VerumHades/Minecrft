#ifndef MESH_HEADER
#define MESH_HEADER

#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class VertexFormat{
    private:
        std::vector<uint> sizes = {};
        uint totalSize = 0;
    public:
        VertexFormat(){};
        VertexFormat(std::vector<uint> sizes);
        void apply();
        uint getVertexSize(){return totalSize;}
};

class Mesh{
    private:
        std::vector<float> vertices;
        std::vector<uint> indices;
        VertexFormat format;

        int textureIndex = -1;

    public:
        size_t vertexSize = 0;

        Mesh(){}

        void setVertexFormat(const VertexFormat& format) {this->format = format;};
        void addQuadFaceGreedy(glm::vec3 vertices_[4], int normal, float vertexOcclusion[4], float textureIndex, int clockwise, int width, int height);
        void addQuadFace(glm::vec3 vertices[4], glm::vec3 normals[4], int clockwise);

        std::vector<float>& getVertices() {return this->vertices;}
        std::vector<uint>& getIndices() {return this->indices;}
        VertexFormat& getFormat() {return this->format;}

        void setTextureIndex(int index){textureIndex = index;}
        int getTextureIndex(){return textureIndex;};
};
#endif