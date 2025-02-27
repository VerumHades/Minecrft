#include <rendering/model.hpp>

#include <filesystem>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <path_config.hpp>

class GenericModel: public Model{
    private:
        std::string directory;
        float pixel_perfect_sampling;

        void processNode(aiNode *node, const aiScene *scene);
        Mesh processMesh(aiMesh *mesh, const aiScene *scene);
        std::shared_ptr<GLTexture2D> loadMaterialTexture(aiMaterial *mat, aiTextureType type);
        
    public:
        GenericModel(const std::string& path, float pixel_perfect_sampling = false);
};