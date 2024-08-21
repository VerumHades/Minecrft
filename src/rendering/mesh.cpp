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
    int vecIndicies[4];
        
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
            {textureX, textureY},
            {textureX + width, textureY},
            {textureX + width, textureY + height},
            {textureX, textureY + height},
        };
        vertex[6] = textureCoordinates[i][0];
        vertex[7] = textureCoordinates[i][1];

        vertex.insert(vertex.begin() + 8, metadata.begin() + 2, metadata.end());

        vecIndicies[i] = this->vertices.size() / this->vertexSize;
        this->vertices.insert(this->vertices.begin() + this->vertices.size(), vertex.begin(), vertex.end());
    }

    if(clockwise){
        this->indices.push_back(vecIndicies[0]);
        this->indices.push_back(vecIndicies[1]);
        this->indices.push_back(vecIndicies[3]);

        this->indices.push_back(vecIndicies[1]);
        this->indices.push_back(vecIndicies[2]);
        this->indices.push_back(vecIndicies[3]);
    }
    else{
        this->indices.push_back(vecIndicies[3]);
        this->indices.push_back(vecIndicies[1]);
        this->indices.push_back(vecIndicies[0]);

        this->indices.push_back(vecIndicies[3]);
        this->indices.push_back(vecIndicies[2]);
        this->indices.push_back(vecIndicies[1]);
    }
}   

/*
    textureX, textureY
    textureX + textureSize, textureY,
    textureX + textureSize, textureY + textureSize
    textureX, textureY + textureSize
*/