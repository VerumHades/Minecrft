#include <rendering/wireframes.hpp>

WireframeCubeRenderer::WireframeCubeRenderer(){
    std::array<float,8 * 3> vertices = {
        0.0f,1.0f,0.0f,
        1.0f,1.0f,0.0f,
        1.0f,0.0f,0.0f,
        0.0f,0.0f,0.0f,
        0.0f,1.0f,1.0f,
        1.0f,1.0f,1.0f,
        1.0f,0.0f,1.0f,
        0.0f,0.0f,1.0f
    };
    vertexBuffer.initialize(vertices.size());
    vertexBuffer.insert(0, vertices.size(), vertices.data());
    vertexBuffer.bind();


    std::array<uint, 8 * 3> indices = {
        0,1, 1,2, 2,3, 3,0, // front
        4,5, 5,6, 6,7, 7,4, // back
        0,4, 1,5, 2,6, 3,7, // front to back connections
    };
    indexBuffer.initialize(indices.size());
    indexBuffer.insert(0, indices.size(), indices.data());

    instanceBuffer.initialize(maxCubes * (3 + 3 + 3));
    instanceBuffer.bind();

    vao.attachBuffer(&vertexBuffer, {VEC3});
    vao.attachBuffer(&instanceBuffer, {VEC3, VEC3, VEC3}, true);
    vao.attachIndexBuffer(&indexBuffer);
}

void WireframeCubeRenderer::setCube(size_t index, glm::vec3 position, glm::vec3 scale, glm::vec3 color){
    float data[9];
    std::memcpy(data, glm::value_ptr(position), 3 * sizeof(float));
    std::memcpy(data + 3,glm::value_ptr(scale), 3 * sizeof(float));
    std::memcpy(data + 6,glm::value_ptr(color), 3 * sizeof(float));

    instanceBuffer.insert(index * 9, 9, data);
    cubes = std::max(cubes, index + 1);
}

void WireframeCubeRenderer::removeCube(size_t index){
    float data[9] = {0,0,0,0,0,0,0,0,0};
    instanceBuffer.insert(index * 9, 9, data);
}

void WireframeCubeRenderer::draw(){
    vao.bind();
    wireframeProgram.updateUniforms();

    glDrawElementsInstanced(GL_LINES, indexBuffer.size(), GL_UNSIGNED_INT, 0, cubes);

    vao.unbind();
}