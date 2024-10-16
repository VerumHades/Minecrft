#include <rendering/mesh.hpp>

VertexFormat::VertexFormat(std::vector<uint32_t> sizes){
    this->sizes = sizes;
    for(auto& size: sizes) totalSize += size;
}

void VertexFormat::apply(){
    int vertexSize = 0;
    for(auto& sz: sizes) vertexSize += sz;

    size_t stride =  vertexSize * sizeof(float);

    size_t size_to_now = 0;
    for(int i = 0;i < sizes.size();i++){
        size_t current_size = sizes[i];
        
        uintptr_t pointer = size_to_now * sizeof(float);

        //printf("Size: %lu Pointer: %lu Stride: %lu\n",current_size,pointer,stride);

        glVertexAttribPointer(i, (int) current_size, GL_FLOAT, GL_FALSE, (int)stride, (void*)pointer);
        glEnableVertexAttribArray(i);

        size_to_now += current_size;
    }
}

Mesh::Mesh(){
    this->vertices.reserve(10000);
    this->indices.reserve(10000);
}

void Mesh::addQuadFaceGreedy(glm::vec3 vertices_[4], int normal, float vertexOcclusion[4], float textureIndex, int clockwise, int width, int height){
    const int greedyVertexSize = 8;
    
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
        vertex[3 + offset] = static_cast<float>(normal);

        // Texture coordinates
        vertex[4 + offset] = textureCoordinates[i].x;
        vertex[5 + offset] = textureCoordinates[i].y;

        vertex[6 + offset] = textureIndex;
        vertex[7 + offset] = vertexOcclusion[i];
        // Store the vertex index and add the vertex to the vertices array
        vecIndices[i] = startIndex + i;
    }

    this->vertices.insert(this->vertices.end(), vertex, vertex + greedyVertexSize * 4);

    if (clockwise) this->indices.insert(this->indices.end(), {vecIndices[0], vecIndices[1], vecIndices[3], vecIndices[1], vecIndices[2], vecIndices[3]});
    else this->indices.insert(this->indices.end(), {vecIndices[3], vecIndices[1], vecIndices[0], vecIndices[3], vecIndices[2], vecIndices[1]});
    
}

void Mesh::addQuadFace(glm::vec3 vertices_[4], glm::vec3 normals[4], int clockwise){
    uint32_t vecIndices[4];
    
    const int size = 7;
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