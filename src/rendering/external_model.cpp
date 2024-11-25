#include <rendering/external_model.hpp>

void ExternalModel::draw(){
    int currentTexture = -1;
    for(int i = 0;i < buffers.size();i++){
        int tindex = textureIndices[i];

        if(tindex != currentTexture && tindex != -1){
            getTexture(tindex).bind(0);
            currentTexture = tindex;
        }

        buffers[i]->draw();
    }
}

void ExternalModel::processNode(aiNode *node, const aiScene *scene)
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

Mesh ExternalModel::processMesh(aiMesh *mesh, const aiScene *scene){
    Mesh outMesh = Mesh();
    outMesh.setVertexFormat(VertexFormat({3,3,2}));
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

    //std::cout << mesh->mMaterialIndex << std::endl;
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        // Retrieve the texture (e.g., the diffuse texture)
        aiString texturePath;
        const aiTextureType typesToCheck[] = { aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE, aiTextureType_UNKNOWN };
        bool textureFound = false;
        for (const aiTextureType type : typesToCheck) {
            if (material->GetTexture(type, 0, &texturePath) == AI_SUCCESS) {
                textureFound = true;
                outMesh.setTextureIndex(handleTexture(texturePath.C_Str(), outMesh, scene));
                break;
            }
        }

        if(!textureFound){
            std::cout << "Warning: No texture found for this material." << std::endl;
            std::string defaultTexturePath = "textures//pig.png";
            outMesh.setTextureIndex(handleTexture(defaultTexturePath.c_str(), outMesh, scene, true));
        }
    }

    return outMesh;
}

int ExternalModel::handleTexture(const char* texturePath, Mesh& outMesh, const aiScene *scene, bool outsideModelPath){
    if (texturePath[0] == '*') {
        // Handling embedded texture
        int textureIndex = atoi(texturePath + 1);
        if (textureIndex >= 0 && textureIndex < scene->mNumTextures) {
            loadTexture(textureIndex, scene);
            return textureIndex;
        }
    } else {
        std::size_t hashValue = std::hash<std::string>{}(texturePath);
        int index = static_cast<int>(hashValue); 

        if(!outsideModelPath){
            std::filesystem::path fullPath = rootPath;
            fullPath /= texturePath;

            loadTexture(index, fullPath.string().c_str());
        }
        else loadTexture(index, texturePath);

        return index;
    }

    return -1;
}

void ExternalModel::loadTexture(int index, const char* filepath){
    if(loadedTextures.count(index) != 0) return;

    int width, height, channels;
    unsigned char* data = stbi_load(
        filepath,
        &width, 
        &height, 
        &channels, 
        4 // force 4 channels (RGBA)
    );

    if (data) {
        // Successfully loaded image data, now use it for rendering
        std::cout << "Loading texture: " << filepath << " with index: " << index << std::endl;
        loadedTextures.try_emplace(index, data, width, height);
        stbi_image_free(data); // Don't forget to free the image data
    } else {
        std::cout << "Failed to load texture:" << filepath << std::endl;
    }
}

void ExternalModel::loadTexture(int index, const aiScene *scene){
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

bool ExternalModel::loadFromFile( const std::string& pFile, const std::string& rootPath) {
    // Create an instance of the Importer class
    Assimp::Importer importer;
    this->rootPath = rootPath;

    // And have it read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll
    // probably to request more postprocessing than we do in this example.

    unsigned int importFlags = 
                            aiProcess_GenNormals  |
                            aiProcess_Triangulate |
                            aiProcess_PreTransformVertices |
                            //aiProcess_JoinIdenticalVertices |
                            //aiProcess_SortByPType |
                            aiProcess_OptimizeMeshes |
                            //aiProcess_OptimizeGraph |
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

    if(meshes.size() == 0){
        std::cout << "ExternalModel is missing a mesh!" << std::endl;
        return false;
    }

    for(int i = 0;i < meshes.size();i++){
        buffers.push_back(std::make_unique<GLBufferLegacy>());
        buffers[buffers.size() - 1]->loadMesh(meshes[i]);
        textureIndices.push_back(meshes[i].getTextureIndex());
    }
    /*for(int i = 0;i < meshes.size();i++){
        GLBufferLegacy& buffer = buffers.emplace_back();
        buffer.loadMesh(meshes[i]);
    }*/

    // We're done. Everything will be cleaned up by the importer destructor
    return true;
}

void ExternalModelManager::initialize(){
    modelProgram = std::make_unique<ShaderProgram>();
    modelDepthProgram = std::make_unique<ShaderProgram>();

    modelProgram->addShader("shaders/graphical/model/model.vs", GL_VERTEX_SHADER);
    modelProgram->addShader("shaders/graphical/model/model.fs", GL_FRAGMENT_SHADER);
    modelProgram->compile();
    
    modelDepthProgram->addShader("shaders/graphical/depth.vs", GL_VERTEX_SHADER);
    modelDepthProgram->addShader("shaders/graphical/depth.fs", GL_FRAGMENT_SHADER);
    modelDepthProgram->compile();

    glUniform1i(modelProgram->getUniformLocation("texture"),0);
    glUniform1i(modelProgram->getUniformLocation("shadowMap"),1);
}

ExternalModel& ExternalModelManager::createModel(std::string name){
    models.emplace(name, *this);
    return getModel(name);
}

void ExternalModelManager::drawModel(ExternalModel& model, Camera& camera, glm::vec3 position, bool depthMode){
    camera.setModelPosition(position);
    //camera.setModelRotation(glm::vec3(-90,0,0));

    if(depthMode) modelDepthProgram->updateUniforms();
    else modelProgram->updateUniforms();

    model.draw();
    camera.setModelRotation(glm::vec3(0,0,0));
}
