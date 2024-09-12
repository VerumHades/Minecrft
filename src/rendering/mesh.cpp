#include <rendering/mesh.hpp>

Mesh::Mesh(){
    //this->vertices.reserve(10000);
    //this->indices.reserve(10000);
}

void Mesh::setVertexFormat(const std::vector<int>& format_){
    this->format = format_;
    this->formatSet = true;

    this->vertexSize = 0; 
    for(const int& i: this->format) this->vertexSize += i;
}

static const int greedyVertexSize = 10;
void Mesh::addQuadFaceGreedy(glm::vec3 vertices_[4], glm::vec3 normals[4], float vertexOcclusion[4], float textureIndex, int clockwise, int width, int height){
    // Precalculate texture coordinates
    float textureX = 1.0;
    float textureY = 1.0;
    float textureXW = textureX + width;
    float textureYH = textureY + height;
    
    glm::vec2 textureCoordinates[4] = {
        {textureX , textureY },
        {textureXW, textureY },
        {textureXW, textureYH},
        {textureX , textureYH}
    };
    

    //this->vertices.reserve(this->vertices.size() + 4 * this->vertexSize);
    //this->indices.reserve(this->indices.size() + 6);

    uint32_t vecIndices[4];

    float vertex[greedyVertexSize * 4];
    uint32_t startIndex = (uint32_t) this->vertices.size() / greedyVertexSize;
    for(int i = 0; i < 4; i++){
        int offset = i * greedyVertexSize;

        vertex[0 + offset] = vertices_[i].x;
        vertex[1 + offset] = vertices_[i].y;
        vertex[2 + offset] = vertices_[i].z;

        // Normals
        vertex[3 + offset] = normals[i].x;
        vertex[4 + offset] = normals[i].y;
        vertex[5 + offset] = normals[i].z;

        // Texture coordinates
        vertex[6 + offset] = textureCoordinates[i].x;
        vertex[7 + offset] = textureCoordinates[i].y;

        vertex[8 + offset] = textureIndex;
        vertex[9 + offset] = vertexOcclusion[i];
        // Store the vertex index and add the vertex to the vertices array
        vecIndices[i] = startIndex + i;
    }

    this->vertices.insert(this->vertices.end(), vertex, vertex + greedyVertexSize * 4);

    if (clockwise) this->indices.insert(this->indices.end(), {vecIndices[0], vecIndices[1], vecIndices[3], vecIndices[1], vecIndices[2], vecIndices[3]});
    else this->indices.insert(this->indices.end(), {vecIndices[3], vecIndices[1], vecIndices[0], vecIndices[3], vecIndices[2], vecIndices[1]});
    
}

static const int size = 7;
void Mesh::addQuadFace(glm::vec3 vertices_[4], glm::vec3 normals[4], int clockwise){
    uint32_t vecIndices[4];

    float vertex[size * 4];
    uint32_t startIndex = (uint32_t) this->vertices.size() / size;
    for(int i = 0; i < 4; i++){
        int offset = i * size;

        vertex[0 + offset] = vertices_[i].x;
        vertex[1 + offset] = vertices_[i].y;
        vertex[2 + offset] = vertices_[i].z;

        // Normals
        vertex[3 + offset] = normals[i].x;
        vertex[4 + offset] = normals[i].y;
        vertex[5 + offset] = normals[i].z;

        vertex[6 + offset] = static_cast<float>(i);

        vecIndices[i] = startIndex + i;
    }

    this->vertices.insert(this->vertices.end(), vertex, vertex + size * 4);

    if (clockwise) this->indices.insert(this->indices.end(), {vecIndices[0], vecIndices[1], vecIndices[3], vecIndices[1], vecIndices[2], vecIndices[3]});
    else this->indices.insert(this->indices.end(), {vecIndices[3], vecIndices[1], vecIndices[0], vecIndices[3], vecIndices[2], vecIndices[1]});
}
/*
    textureX, textureY
    textureX + textureSize, textureY,
    textureX + textureSize, textureY + textureSize
    textureX, textureY + textureSize
*/