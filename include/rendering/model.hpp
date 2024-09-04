#ifndef MODEL_H
#define MODEL_H

#include <memory>
#include <vector>

#include <rendering/mesh.hpp>
#include <rendering/camera.hpp>
#include <rendering/buffer.hpp>

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

#define MAX_MODEL_CUBOIDS 64

class Cuboid{
    private:
        glm::vec3 offset;
        glm::vec3 dimensions;
};

class ModelManager;

class Model{
    private:
        std::vector<Cuboid> cuboids;
        ModelManager& manager;

    public:
        Model(ModelManager& manager) : manager(manager) {}
        const std::vector<Cuboid>& getCuboids();
};

class ModelManager{
    private:
        std::unordered_map<std::string, Model> models;
        GLBuffer cubeBuffer;
        ShaderProgram modelProgram;

    public:
        void initialize();
        Model& createModel(std::string name);
        Model& getModel(std::string name) {return models.at(name);}
        void drawModel(Model& model, Camera& camera, glm::vec3 offset);
};

#endif