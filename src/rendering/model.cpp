#include <rendering/model.hpp>

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
    outMesh.setVertexFormat({3,3});
    const int size = 9;

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

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Retrieve the texture (e.g., the diffuse texture)
        aiString texturePath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
            // Load the texture using your GLTexture class
            outMesh.setTexture(texturePath.C_Str());
        }
    }

    return outMesh;
}

bool Model::loadFromFile( const std::string& pFile) {
  // Create an instance of the Importer class
  Assimp::Importer importer;

  // And have it read the given file with some example postprocessing
  // Usually - if speed is not the most important aspect for you - you'll
  // probably to request more postprocessing than we do in this example.
  const aiScene* scene = importer.ReadFile( pFile,
    aiProcess_CalcTangentSpace       |
    aiProcess_Triangulate            |
    aiProcess_JoinIdenticalVertices  |
    aiProcess_SortByPType);

  // If the import failed, report it
  if (nullptr == scene) {
    std::cout << importer.GetErrorString() << std::endl;
    return false;
  }

  // Now we can access the file's contents.
  processNode(scene->mRootNode, scene);

  // We're done. Everything will be cleaned up by the importer destructor
  return true;
}


Model& ModelManager::createModel(std::string name){
    models.emplace(name, *this);
    return getModel(name);
}

void ModelManager::drawModel(Model& model, Camera& camera, glm::vec3 position, bool depthMode){
    
}