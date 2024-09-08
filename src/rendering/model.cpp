#include <rendering/model.hpp>

FaceDefinition faceDefinitions[] = {
    FaceDefinition(0, 1, 0, {4, 5, 1, 0}, 0),              // Top face
    FaceDefinition(0, -1, 0, {7, 6, 2, 3}, 1, true),       // Bottom face
    FaceDefinition(-1, 0, 0, {0, 4, 7, 3}, 2, true),       // Left face
    FaceDefinition(1, 0, 0, {1, 5, 6, 2}, 3),              // Right face
    FaceDefinition(0, 0, -1, {0, 1, 2, 3}, 4),             // Front face
    FaceDefinition(0, 0, 1, {4, 5, 6, 7}, 5, true)         // Back face
};

const glm::vec3 cubeNormals[8] = {
    glm::normalize(glm::vec3(-1, 1,-1)),
    glm::normalize(glm::vec3( 1, 1,-1)),
    glm::normalize(glm::vec3( 1,-1,-1)),
    glm::normalize(glm::vec3(-1,-1,-1)),
    glm::normalize(glm::vec3(-1, 1, 1)),
    glm::normalize(glm::vec3( 1, 1, 1)),
    glm::normalize(glm::vec3( 1,-1, 1)),
    glm::normalize(glm::vec3(-1,-1, 1))
};

Mesh generateBasicCubeMesh(){
    Mesh mesh = Mesh();
    mesh.setVertexFormat({3,3,1});

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

        glm::vec3 normalArray[4] = {
            cubeNormals[def.vertexIndexes[0]],
            cubeNormals[def.vertexIndexes[1]],
            cubeNormals[def.vertexIndexes[2]],
            cubeNormals[def.vertexIndexes[3]]
        };

        mesh.addQuadFace(
            vertexArray,
            normalArray,
            def.clockwise
        );
    }

    return mesh;
}

void Model::calculateMatrices(){
    auto cubos = this->getCuboids();
    
    calculatedMatrices.clear();
    calculatedTextureMatrices.clear();

    //mat[col][row]

    for(auto& cuboid: cubos){
        glm::mat4 temp = glm::mat4(1.0f);

        temp = glm::scale(temp, cuboid.dimensions);        
        temp = glm::translate(glm::mat4(1.0f), cuboid.offset) * temp;

        calculatedMatrices.push_back(temp);
        
        glm::mat3 texTemp = glm::mat3(1.0f);

        texTemp[0][0] = cuboid.textureCoordinates[0].x;
        texTemp[1][0] = cuboid.textureCoordinates[0].x;

        texTemp[2][0] = cuboid.textureCoordinates[1].x;
        texTemp[0][1] = cuboid.textureCoordinates[1].x;

        texTemp[1][1] = cuboid.textureCoordinates[2].x;
        texTemp[2][1] = cuboid.textureCoordinates[2].x;

        texTemp[0][2] = cuboid.textureCoordinates[3].x;
        texTemp[1][2] = cuboid.textureCoordinates[3].x;

        texTemp[2][2] = static_cast<float>(cuboid.textureIndex);

        calculatedTextureMatrices.push_back(texTemp);
    }
}

void Model::setCuboids(std::vector<Cuboid> cuboids_){
    cuboids = cuboids_;
    calculateMatrices();
}

void ModelManager::initialize(){
    Mesh mesh = generateBasicCubeMesh();

    modelProgram = std::make_unique<ShaderProgram>();

    modelProgram->initialize();
    modelProgram->addShader("shaders/model/model.vs", GL_VERTEX_SHADER);
    modelProgram->addShader("shaders/fragment.fs", GL_FRAGMENT_SHADER);
    modelProgram->compile();
    modelProgram->use();

    cubiodMatUniform.attach(*modelProgram);
    cubiodTexUniform.attach(*modelProgram);


    /*glValidateProgram(modelProgram->getID());
    GLint validateStatus;
    glGetProgramiv(modelProgram->getID(), GL_VALIDATE_STATUS, &validateStatus);
    if (!validateStatus) {
        char infoLog[512];
        glGetProgramInfoLog(modelProgram->getID(), 512, NULL, infoLog);
        std::cerr << "Shader Program Validation Error: " << infoLog << std::endl;
    }*/

    cubeBuffer = std::make_unique<GLBuffer>();

    std::cout << "Loading mesh: "  << mesh.getVertices().size() << std::endl;

    cubeBuffer->loadMesh(mesh);

    modelDepthProgram = std::make_unique<ShaderProgram>();

    modelDepthProgram->initialize();
    modelDepthProgram->addShader("shaders/model/depthModel.vs", GL_VERTEX_SHADER);
    modelDepthProgram->addShader("shaders/depth.fs", GL_FRAGMENT_SHADER);
    modelDepthProgram->compile();
    modelDepthProgram->use();

    cubiodMatUniform.attach(*modelDepthProgram);
}

Model& ModelManager::createModel(std::string name){
    models.emplace(name, *this);
    return getModel(name);
}

void ModelManager::drawModel(Model& model, Camera& camera, glm::vec3 position, bool depthMode){
    cubiodMatUniform = model.getCalculatedMatrices();
    cubiodTexUniform = model.getCalculatedTextureMatrices();

    camera.setModelPosition(position);
    
    if(!depthMode) this->modelProgram->updateUniforms();
    else this->modelDepthProgram->updateUniforms();

    this->cubeBuffer->drawInstances(static_cast<int>(model.getCalculatedMatrices().size()));
}