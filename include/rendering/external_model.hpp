#ifndef MODEL_H
#define MODEL_H

#include <memory>
#include <vector>
#include <filesystem> //C++ 17
#include <functional> // For std::hash

#include <rendering/camera.hpp>
#include <rendering/buffer.hpp>
#include <rendering/mesh.hpp>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <stb_image.h>

class ExternalModelManager;

class ExternalModel{
    private:
        std::unordered_map<int, GLTexture> loadedTextures;

        ExternalModelManager& manager;
        std::string rootPath;
        
        std::vector<Mesh> meshes;
        std::vector<std::unique_ptr<GLBufferLegacy>> buffers;
        std::vector<int> textureIndices;

        void processNode(aiNode *node, const aiScene *scene);
        Mesh processMesh(aiMesh *mesh, const aiScene *scene);
        int handleTexture(const char* texturePath, Mesh& outMesh, const aiScene *scene, bool outsideModelPath = false);

        void loadTexture(int index, const aiScene *scene);
        void loadTexture(int index, const char* filepath);

    public:
        ExternalModel(ExternalModelManager& manager) : manager(manager) {}
        bool loadFromFile(const std::string& filename, const std::string& rootPath);
        GLTexture& getTexture(int index) {return loadedTextures.at(index);};
        void draw();
};

class ExternalModelManager{
    private:
        std::unordered_map<std::string, ExternalModel> models;
        std::unique_ptr<ShaderProgram> modelProgram;
        std::unique_ptr<ShaderProgram> modelDepthProgram;

    public:
        void initialize();

        ExternalModel& createModel(std::string name);
        ExternalModel& getModel(std::string name) {
            if(models.count(name) == 0) {
                std::cout << "ExternalModel: " << name << "not really found." << std::endl;
            }
            return models.at(name);
        }
        void drawModel(ExternalModel& model, Camera& camera, glm::vec3 offset, bool depthMode = false);

        ShaderProgram& getModelProgram() {return *modelProgram;};
        ShaderProgram& getModelDepthProgram() {return *modelDepthProgram;};
};

#endif