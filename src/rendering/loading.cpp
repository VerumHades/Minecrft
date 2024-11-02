#include <rendering/loading.hpp>

ChunkLoadingVisualizer::ChunkLoadingVisualizer(){
    Mesh mesh = Mesh();
    mesh.setVertexFormat(VertexFormat({3,3}));

    program.initialize();
    program.addShader("shaders/load_screens/chunk.vs", GL_VERTEX_SHADER);
    program.addShader("shaders/load_screens/chunk.fs", GL_FRAGMENT_SHADER);
    program.compile();
    program.use();
    
    projectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

    projectionMatrix.attach(program);
    viewMatrix.attach(program);
    modelMatrix.attach(program);

    program.updateUniforms();
    //projectionMatrix = glm::perspective<float>(glm::radians(45), aspect, zNear, zFar);

    std::array<float, 8 * 6> vertices = {
        0,1,0, 0,0,0,
        1,1,0, 0,0,0,
        1,0,0, 0,0,0,
        0,0,0, 0,0,0,

        0,1,1, 1,1,1,
        1,1,1, 1,1,1,
        1,0,1, 1,1,1,
        0,0,1, 1,1,1
    };

    std::array<GLuint,6 * 6> indices = {
        0,1,3, // Front face
        1,3,2,
        4,5,7, // Back face
        5,6,7,
        0,4,3, // Left face
        4,7,3,
        1,5,2, // Right face
        5,6,2,
        0,1,4, // Top face
        4,5,1,
        3,7,2, // Bottom face
        7,6,2
    };

    mesh.getVertices().insert(mesh.getVertices().end(), vertices.begin(), vertices.end());
    mesh.getIndices().insert(mesh.getIndices().end(), indices.begin(), indices.end());

    cubeBuffer.loadMesh(mesh);
}

void ChunkLoadingVisualizer::setup(int screenWidth, int screenHeight){
    float aspect = (float) screenWidth / (float) screenHeight;

    projectionMatrix = glm::perspective(glm::radians(90.0f), aspect, 0.1f, 400.0f);
    program.updateUniforms();
}

void ChunkLoadingVisualizer::render(int renderDistance){
    this->rotation = (this->rotation + 1) % 3600;

    float x = 1, z = 1;
    x = sin(glm::radians(static_cast<float>(rotation) / 10.0f));
    z = cos(glm::radians(static_cast<float>(rotation) / 10.0f));

    float distance = 20.0f;

    viewMatrix = glm::lookAt(glm::vec3(x * distance,distance,z * distance), {0,0,0}, {0,1,0});
    program.updateUniforms();

    for(auto& pos: loadedChunks){
        modelMatrix = glm::translate(glm::mat4(1), glm::vec3(pos));
        program.updateUniform("modelMatrix");

        cubeBuffer.draw();
    }
}