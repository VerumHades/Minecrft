#ifndef MODEL_H
#define MODEL_H

#include <memory>
#include <vector>

#include <rendering/camera.hpp>
#include <rendering/buffer.hpp>
#include <rendering/mesh.hpp>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <stb_image.h>

struct FaceDefinition {
    int offsetX = 0;
    int offsetY = 0;
    int offsetZ = 0;
    std::vector<int> vertexIndexes;
    int textureIndex = 0;
    bool clockwise = false;

    FaceDefinition(int offsetX, int offsetY, int offsetZ, 
                   std::vector<int> vertexIndexes, int textureIndex, 
                   bool clockwise = false)
        : offsetX(offsetX), offsetY(offsetY), offsetZ(offsetZ),
          vertexIndexes(std::move(vertexIndexes)), textureIndex(textureIndex),
          clockwise(clockwise) {}
};

extern FaceDefinition faceDefinitions[];
extern const glm::vec3 cubeNormals[8];

#define MAX_MODEL_CUBOIDS 64

class Cuboid{
    public:
        glm::vec3 offset;
        glm::vec3 dimensions;

        glm::vec2 textureCoordinates[4];
        int textureIndex;
};

class ModelManager;

class Model{
    private:
        ModelManager& manager;
        
        std::vector<Mesh> meshes;
        std::vector<std::unique_ptr<GLBuffer>> buffers;

        void processNode(aiNode *node, const aiScene *scene);
        Mesh processMesh(aiMesh *mesh, const aiScene *scene);
        int handleTexture(aiString texturePath, Mesh& outMesh, const aiScene *scene);

    public:
        int textureIndex = -1;
        
        Model(ModelManager& manager) : manager(manager) {}
        bool loadFromFile(const std::string& filename);
        void draw();
};

class ModelManager{
    private:
        std::unordered_map<std::string, Model> models;
        std::unique_ptr<ShaderProgram> modelProgram;
        std::unique_ptr<ShaderProgram> modelDepthProgram;
        std::unordered_map<int, GLTexture> loadedTextures;

    public:
        void initialize();
        void loadTexture(int index, const aiScene *scene);
        GLTexture& getTexture(int index) {return loadedTextures.at(index);};

        Model& createModel(std::string name);
        Model& getModel(std::string name) {
            if(models.count(name) == 0) {
                std::cout << "Model: " << name << "not really found." << std::endl;
            }
            return models.at(name);
        }
        void drawModel(Model& model, Camera& camera, glm::vec3 offset, bool depthMode = false);

        ShaderProgram& getModelProgram() {return *modelProgram;};
        ShaderProgram& getModelDepthProgram() {return *modelDepthProgram;};
};

#endif