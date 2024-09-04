#include <rendering/model.hpp>

FaceDefinition faceDefinitions[] = {
    FaceDefinition(0, 1, 0, {4, 5, 1, 0}, 0),              // Top face
    FaceDefinition(0, -1, 0, {7, 6, 2, 3}, 1, true),       // Bottom face
    FaceDefinition(-1, 0, 0, {0, 4, 7, 3}, 2, true),       // Left face
    FaceDefinition(1, 0, 0, {1, 5, 6, 2}, 3),              // Right face
    FaceDefinition(0, 0, -1, {0, 1, 2, 3}, 4),             // Front face
    FaceDefinition(0, 0, 1, {4, 5, 6, 7}, 5, true)         // Back face
};

Mesh generateBasicCubeMesh(){
    Mesh mesh = Mesh();
    mesh.setVertexFormat({3,3});

    glm::vec3 vertices[] = {
        {0,1,0},
        {1,1,0},
        {1,0,0},
        {0,0,0},
        {0,1,1},
        {1,1,1},
        {1,0,1},
        {0,0,1}
    }; // cube vertices

    for(int i = 0;i < 6;i++){
        const FaceDefinition& def = faceDefinitions[i];
       
        glm::vec3 vertexArray[4] = {
            vertices[def.vertexIndexes[0]],
            vertices[def.vertexIndexes[1]],
            vertices[def.vertexIndexes[2]],
            vertices[def.vertexIndexes[3]]
        };

        mesh.addQuadFace(
            vertexArray,
            glm::vec3(def.offsetX, def.offsetY,  def.offsetZ),
            def.clockwise
        );
    }

    return mesh;
}

void ModelManager::initialize(){
    cubeBuffer.loadMesh(generateBasicCubeMesh());
    
    modelProgram.initialize();
    modelProgram.addShader("shaders/model/model.vs", GL_VERTEX_SHADER);
    modelProgram.addShader("shaders/model/model.fs", GL_FRAGMENT_SHADER);
    modelProgram.compile();
}

Model& ModelManager::createModel(std::string name){
    models.emplace(name, *this);
    return getModel(name);
}

void ModelManager::drawModel(Model& model, Camera& camera, glm::vec3 offset){
    auto cubos = model.getCuboids();
    modelProgram.use();

    
}