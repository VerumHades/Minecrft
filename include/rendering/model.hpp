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
        bool loadFromFile(const std::string& filename);
        void processNode(aiNode *node, const aiScene *scene);
        Mesh processMesh(aiMesh *mesh, const aiScene *scene);

    public:
        Model(ModelManager& manager, const std::string& filename) : manager(manager) {}
};

class ModelManager{
    private:
        std::unordered_map<std::string, Model> models;

    public:
        void initialize();
        Model& createModel(std::string name);
        Model& getModel(std::string name) {
            if(models.count(name) == 0) {
                std::cout << "Model: " << name << "not really found." << std::endl;
            }
            return models.at(name);
        }
        void drawModel(Model& model, Camera& camera, glm::vec3 offset, bool depthMode = false);
};

#endif