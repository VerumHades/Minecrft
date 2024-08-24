#include <rendering/mesh.hpp>

Mesh::Mesh(){
    this->vertices.reserve(5000);
    this->indices.reserve(5000);
}
const std::vector<float>& Mesh::getVertices(){
    return this->vertices;
}
const std::vector<unsigned int>& Mesh::getIndices(){
    return this->indices;
}
const std::vector<int>& Mesh::getFormat(){
    return this->format;
}

void Mesh::setVertexFormat(const std::vector<int>& format){
    this->format = format;
    this->formatSet = true;

    this->vertexSize = 0; 
    for(const int& i: this->format) this->vertexSize += i;
}

void Mesh::addQuadFace(glm::vec3 vertices[4], glm::vec3 normals, std::vector<float> metadata, int clockwise, int width, int height){
    unsigned int vecIndices[4];
    
    for(int i = 0;i < 4;i++){
        glm::vec3 pos = vertices[i];

        std::vector<float> vertex(this->vertexSize, 0.0f);
        
        vertex[0] = pos.x;
        vertex[1] = pos.y;
        vertex[2] = pos.z;

        vertex[3] = normals.x;
        vertex[4] = normals.y;
        vertex[5] = normals.z;

        float textureX = metadata[0];
        float textureY = metadata[1];

        std::vector<std::vector<float>> textureCoordinates = {
            {textureX        , textureY         },
            {textureX + width, textureY         },
            {textureX + width, textureY + height},
            {textureX        , textureY + height}
        };
        vertex[6] = textureCoordinates[i][0];
        vertex[7] = textureCoordinates[i][1];

        vertex[8] = metadata[2];

        vecIndices[i] = this->vertices.size() / this->vertexSize;
        this->vertices.insert(this->vertices.end(), vertex.begin(), vertex.end());
    }

    if (clockwise) {
        this->indices.insert(this->indices.end(), {vecIndices[0], vecIndices[1], vecIndices[3]});
        this->indices.insert(this->indices.end(), {vecIndices[1], vecIndices[2], vecIndices[3]});
    } else {
        this->indices.insert(this->indices.end(), {vecIndices[3], vecIndices[1], vecIndices[0]});
        this->indices.insert(this->indices.end(), {vecIndices[3], vecIndices[2], vecIndices[1]});
    }
}   

/*
    textureX, textureY
    textureX + textureSize, textureY,
    textureX + textureSize, textureY + textureSize
    textureX, textureY + textureSize
*/