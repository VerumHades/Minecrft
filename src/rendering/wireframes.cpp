#include <rendering/wireframes.hpp>

WireframeCubeRenderer::WireframeCubeRenderer(){
    vao.bind();

    wireframeProgram.initialize();
    wireframeProgram.addShader("shaders/graphical/wireframe/cube_wireframe.vs", GL_VERTEX_SHADER);
    wireframeProgram.addShader("shaders/graphical/wireframe/cube_wireframe.fs", GL_FRAGMENT_SHADER);
    wireframeProgram.compile();
    wireframeProgram.use();

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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    std::array<uint, 8 * 3> indices = {
        0,1, 1,2, 2,3, 3,0, // front
        4,5, 5,6, 6,7, 7,4, // back
        0,4, 1,5, 2,6, 3,7, // front to back connections
    };
    indexBuffer.initialize(indices.size());
    indexBuffer.insert(0, indices.size(), indices.data());

    instanceBuffer.initialize(maxCubes * (3 + 3 + 3));
    instanceBuffer.bind();

    glEnableVertexAttribArray(1); // Position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glVertexAttribDivisor(1, 1);  // Divisor 1 -> advance per instance

    glEnableVertexAttribArray(2); // Scale
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3); // Color
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glVertexAttribDivisor(3, 1);

    vao.unbind();
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
}