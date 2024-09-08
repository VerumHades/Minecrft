#ifndef MODEL_H
#define MODEL_H

#include <memory>
#include <vector>

#include <rendering/camera.hpp>
#include <rendering/buffer.hpp>
#include <rendering/mesh.hpp>

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
        std::vector<Cuboid> cuboids;
        std::vector<glm::mat4> calculatedMatrices;
        std::vector<glm::mat3> calculatedTextureMatrices;
        ModelManager& manager;

    public:
        Model(ModelManager& manager) : manager(manager) {}
        void setCuboids(std::vector<Cuboid> cuboids);
        void calculateMatrices();
        const std::vector<Cuboid>& getCuboids() const {return cuboids;};

        std::vector<glm::mat4> getCalculatedMatrices() {return calculatedMatrices;};
        std::vector<glm::mat3> getCalculatedTextureMatrices() {return calculatedTextureMatrices;};
};

class ModelManager{
    private:
        std::unordered_map<std::string, Model> models;
        std::unique_ptr<GLBuffer> cubeBuffer;
        std::unique_ptr<ShaderProgram> modelProgram;
        std::unique_ptr<ShaderProgram> modelDepthProgram;

        Uniform<std::vector<glm::mat4>> cubiodMatUniform = Uniform<std::vector<glm::mat4>>("cuboidMatrices");
        Uniform<std::vector<glm::mat3>> cubiodTexUniform = Uniform<std::vector<glm::mat3>>("textureCoordinates");

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

        ShaderProgram& getModelProgram() {return *modelProgram;};
        ShaderProgram& getModelDepthProgram() {return *modelDepthProgram;};
};

#endif