#include <game/models/generic_model.hpp>

GenericModel::GenericModel(const std::string& path, float pixel_perfect_sampling): pixel_perfect_sampling(pixel_perfect_sampling){
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

void GenericModel::processNode(aiNode *node, const aiScene *scene)
{
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        Mesh processed_mesh = processMesh(mesh, scene);
        addMesh(processed_mesh);			
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}  


Mesh GenericModel::processMesh(aiMesh *mesh, const aiScene *scene)
{
    Mesh output = createMesh();

    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        std::array<float, 13> vertex_data{};

        vertex_data[0] = mesh->mVertices[i].x;
        vertex_data[1] = mesh->mVertices[i].z;
        vertex_data[2] = mesh->mVertices[i].y; 

        if(mesh->mNormals){
            vertex_data[3] = mesh->mNormals[i].x;
            vertex_data[4] = mesh->mNormals[i].z;
            vertex_data[5] = mesh->mNormals[i].y;
        }

        if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            vertex_data[6] = mesh->mTextureCoords[0][i].x; 
            vertex_data[7] = mesh->mTextureCoords[0][i].y;
        }

        vertex_data[12] = static_cast<float>(pixel_perfect_sampling);
        
        output.getVertices().insert(output.getVertices().end(), vertex_data.begin(), vertex_data.end());
    }

    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        //std::cout << "Face:" << "\n";
        for(unsigned int j = 0; j < face.mNumIndices; j++){
            output.getIndices().push_back(face.mIndices[j]);
            //auto* start = &output.getVertices()[face.mIndices[j] * 13];
            //for(int g = 0;g < 13;g++) std::cout << *(start + g) << " ";
            //std::cout << "\n";
        }
    }  

    if(mesh->mMaterialIndex >= 0)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        
        output.setTexture(loadMaterialTexture(material, aiTextureType_DIFFUSE));
        //textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        //vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR);
        //textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }  

    return output;
}  

std::shared_ptr<GLTexture2D> GenericModel::loadMaterialTexture(aiMaterial *mat, aiTextureType type)
{
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        return std::make_shared<GLTexture2D>(
            (fs::path(directory) / fs::path(str.C_Str())).string()
        );
    }
    return nullptr;
}  