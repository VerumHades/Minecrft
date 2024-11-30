#include <rendering/mesh.hpp>

VertexFormat::VertexFormat(std::vector<uint> sizes){
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

    uint vecIndices[4];

    float vertex[greedyVertexSize * 4];
    uint startIndex = (uint) this->vertices.size() / greedyVertexSize;
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

void Mesh::addQuadFace(std::array<glm::vec3, 4> vertices_, glm::vec3 normal, bool clockwise, std::vector<float> metadata){
    const int size = 8 + metadata.size();
    uint vecIndices[4];
    
    float textureX = 0.0;
    float textureY = 0.0;
    float textureXW = textureX + 1;
    float textureYH = textureY + 1;
    
    glm::vec2 textureCoordinates[4] = {
        {textureX , textureY },
        {textureXW, textureY },
        {textureXW, textureYH},
        {textureX , textureYH}
    };

    std::vector<float> vertex = std::vector<float>(size * 4);
    uint startIndex = (uint) this->vertices.size() / size;
    for(int i = 0; i < 4; i++){
        int offset = i * size;

        vertex[0 + offset] = vertices_[i].x;
        vertex[1 + offset] = vertices_[i].y;
        vertex[2 + offset] = vertices_[i].z;

        vertex[3 + offset] = normal.x;
        vertex[4 + offset] = normal.y;
        vertex[5 + offset] = normal.z;

        vertex[6 + offset] = textureCoordinates[i].x;
        vertex[7 + offset] = textureCoordinates[i].y;

        std::memcpy(vertex.data() + offset + 8, metadata.data(), metadata.size() * sizeof(float));

        vecIndices[i] = startIndex + i;
    }

    this->vertices.insert(this->vertices.end(), vertex.begin(), vertex.end());

    if (clockwise) this->indices.insert(this->indices.end(), {vecIndices[0], vecIndices[1], vecIndices[3], vecIndices[1], vecIndices[2], vecIndices[3]});
    else this->indices.insert(this->indices.end(), {vecIndices[3], vecIndices[1], vecIndices[0], vecIndices[3], vecIndices[2], vecIndices[1]});
}
/*
    textureX, textureY
    textureX + textureSize, textureY,
    textureX + textureSize, textureY + textureSize
    textureX, textureY + textureSize
*/