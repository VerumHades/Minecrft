#include <rendering/model.hpp>

FaceDefinition faceDefinitions[] = {
    FaceDefinition(0, 1, 0, {4, 5, 1, 0}, 0, true),              // Top face
    FaceDefinition(0, -1, 0, {7, 6, 2, 3}, 1),       // Bottom face 1
    FaceDefinition(-1, 0, 0, {0, 4, 7, 3}, 2),       // Left face 1
    FaceDefinition(1, 0, 0, {1, 5, 6, 2}, 3, true),              // Right face
    FaceDefinition(0, 0, -1, {0, 1, 2, 3}, 4, true),             // Front face
    FaceDefinition(0, 0, 1, {4, 5, 6, 7}, 5)         // Back face 1
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


void Model::draw(){
    for(auto& buffer: buffers){
        buffer->draw();
    }
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes.push_back(processMesh(mesh, scene));			
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}  

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene){
    Mesh outMesh = Mesh();
    outMesh.setVertexFormat({3,3,2});
    const int size = 8;

    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        float vertex[size];
        
        vertex[0] = mesh->mVertices[i].x;
        vertex[1] = mesh->mVertices[i].y;
        vertex[2] = mesh->mVertices[i].z; 

        vertex[3] = mesh->mNormals[i].x;
        vertex[4] = mesh->mNormals[i].y;
        vertex[5] = mesh->mNormals[i].z; 

        if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            vertex[6] = mesh->mTextureCoords[0][i].x; 
            vertex[7] = mesh->mTextureCoords[0][i].y;
        }
        else{
            vertex[6] = 0;
            vertex[7] = 0;
        }

        outMesh.getVertices().insert(outMesh.getVertices().end(), vertex, vertex + size);;
    }

    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        outMesh.getIndices().insert(outMesh.getIndices().end(), face.mIndices, face.mIndices + face.mNumIndices);
    }  

    if (mesh->mMaterialIndex >= 0 && outMesh.getTextureIndex() == -1) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Retrieve the texture (e.g., the diffuse texture)
        aiString texturePath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
            outMesh.setTextureIndex(handleTexture(texturePath, outMesh, scene));
        }
    }

    return outMesh;
}

int Model::handleTexture(aiString texturePath, Mesh& outMesh, const aiScene *scene){
    if (texturePath.C_Str()[0] == '*') {
        // Handling embedded texture
        int textureIndex = atoi(texturePath.C_Str() + 1);
        if (textureIndex >= 0 && textureIndex < scene->mNumTextures) {
            manager.loadTexture(textureIndex, scene);
            return textureIndex;
        }

    } else {
        // External texture path
        std::cout << "External textures unsuported." << std::endl;
        //std::cout << texturePath.C_Str();
        //outMesh.setTexture(texturePath.C_Str()); // Make sure to handle this path correctly
    }

    return -1;
}

void ModelManager::loadTexture(int index, const aiScene *scene){
    if(loadedTextures.count(index) != 0) return;
    aiTexture* embeddedTexture = scene->mTextures[index];

    if (embeddedTexture->mHeight == 0) {
        // The texture is compressed (PNG, JPEG, etc.)
        std::cout << "Compressed texture found. Format hint: " << embeddedTexture->achFormatHint << std::endl;
        
        // Here, embeddedTexture->pcData contains the image data
        // embeddedTexture->mWidth is the size of the compressed data in bytes

        // Use stb_image (or any other image library) to decode the compressed image
        int width, height, channels;
        unsigned char* data = stbi_load_from_memory(
            reinterpret_cast<unsigned char*>(embeddedTexture->pcData),
            embeddedTexture->mWidth, 
            &width, 
            &height, 
            &channels, 
            4 // force 4 channels (RGBA)
        );

        if (data) {
            // Successfully loaded image data, now use it for rendering
            loadedTextures.try_emplace(index, data, width, height);
            stbi_image_free(data); // Don't forget to free the image data
        } else {
            std::cout << "Failed to load embedded texture." << std::endl;
        }

    } else {
        // The texture is raw data (uncompressed)
        std::cout << "Raw texture found. Size: " << embeddedTexture->mWidth << "x" << embeddedTexture->mHeight << std::endl;

        // Assuming the texture is RGBA (4 bytes per pixel)
        unsigned char* data = reinterpret_cast<unsigned char*>(embeddedTexture->pcData);
        int width = embeddedTexture->mWidth;
        int height = embeddedTexture->mHeight;

        // Pass the raw texture data to your rendering system
        loadedTextures.try_emplace(index, data, width, height);
    }
}

bool Model::loadFromFile( const std::string& pFile) {
    // Create an instance of the Importer class
    Assimp::Importer importer;

    // And have it read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll
    // probably to request more postprocessing than we do in this example.

    unsigned int importFlags = 
                            aiProcess_GenNormals  |
                            aiProcess_Triangulate |
                            aiProcess_PreTransformVertices |
                            aiProcess_JoinIdenticalVertices |
                            aiProcess_SortByPType |
                            aiProcess_OptimizeMeshes |
                            aiProcess_OptimizeGraph |
                            aiProcess_FlipUVs;
                           //aiProcess_ConvertToLeftHanded;

    //unsigned int importFlags = aiProcess_Triangulate;    

    const aiScene* scene = importer.ReadFile( pFile,importFlags);

    // If the import failed, report it
    if (nullptr == scene) {
        std::cout << importer.GetErrorString() << std::endl;
        return false;
    }

    // Now we can access the file's contents.
    processNode(scene->mRootNode, scene);

    if(meshes.size() > 1){
        std::cout << "Multiple meshes unsuported: " << meshes.size() << std::endl;
        return false;
    }

    if(meshes.size() == 0){
        std::cout << "Model is missing a mesh!" << std::endl;
        return false;
    }

    for(int i = 0;i < meshes.size();i++){
        buffers.push_back(std::make_unique<GLBuffer>());
        buffers[buffers.size() - 1]->loadMesh(meshes[i]);

        if(textureIndex == -1) textureIndex = meshes[i].getTextureIndex();
    }
    /*for(int i = 0;i < meshes.size();i++){
        GLBuffer& buffer = buffers.emplace_back();
        buffer.loadMesh(meshes[i]);
    }*/

    // We're done. Everything will be cleaned up by the importer destructor
    return true;
}

void ModelManager::initialize(){
    modelProgram = std::make_unique<ShaderProgram>();
    modelDepthProgram = std::make_unique<ShaderProgram>();

    modelProgram->initialize();
    modelProgram->addShader("shaders/model/model.vs", GL_VERTEX_SHADER);
    modelProgram->addShader("shaders/model/model.fs", GL_FRAGMENT_SHADER);
    modelProgram->compile();
    
    modelDepthProgram->initialize();
    modelDepthProgram->addShader("shaders/depth.vs", GL_VERTEX_SHADER);
    modelDepthProgram->addShader("shaders/depth.fs", GL_FRAGMENT_SHADER);
    modelDepthProgram->compile();

    glUniform1i(modelProgram->getUniformLocation("texture"),0);
    glUniform1i(modelProgram->getUniformLocation("shadowMap"),1);

    /*glValidateProgram(modelProgram->getID());
    GLint validateStatus;
    glGetProgramiv(modelProgram->getID(), GL_VALIDATE_STATUS, &validateStatus);
    if (!validateStatus) {
        char infoLog[512];
        glGetProgramInfoLog(modelProgram->getID(), 512, NULL, infoLog);
        std::cerr << "Shader Program Validation Error: " << infoLog << std::endl;
    }*/
}

Model& ModelManager::createModel(std::string name){
    models.emplace(name, *this);
    return getModel(name);
}

void ModelManager::drawModel(Model& model, Camera& camera, glm::vec3 position, bool depthMode){
    camera.setModelPosition(position);

    if(depthMode) modelDepthProgram->updateUniforms();
    else modelProgram->updateUniforms();

    if(model.textureIndex != -1){
        getTexture(model.textureIndex).bind(0);
    }

    model.draw();
}
