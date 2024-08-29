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
    // Precalculate texture coordinates
    float textureX = metadata[0];
    float textureY = metadata[1];
    float textureXW = textureX + width;
    float textureYH = textureY + height;
    
    glm::vec2 textureCoordinates[4] = {
        {textureX , textureY },
        {textureXW, textureY },
        {textureXW, textureYH},
        {textureX , textureYH}
    };

    unsigned int vecIndices[4];

    std::vector<float> vertex(this->vertexSize);
    for(int i = 0; i < 4; i++){
        vertex[0] = vertices[i].x;
        vertex[1] = vertices[i].y;
        vertex[2] = vertices[i].z;

        // Normals
        vertex[3] = normals.x;
        vertex[4] = normals.y;
        vertex[5] = normals.z;

        // Texture coordinates
        vertex[6] = textureCoordinates[i].x;
        vertex[7] = textureCoordinates[i].y;

        // Metadata
        vertex[8] = metadata[2];  // Assuming this is some identifier or flag

        // Store the vertex index and add the vertex to the vertices array
        vecIndices[i] = this->vertices.size() / this->vertexSize;
        this->vertices.insert(this->vertices.end(), vertex.begin(), vertex.end());
    }

    // Insert indices depending on winding order (clockwise or counter-clockwise)
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