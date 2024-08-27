#ifndef CAMERA_H
#define CAMERA_H

#include <rendering/shaders.hpp>
#include <rendering/texture.hpp>
#include <optional>
#include <vector>
#include <unordered_map>

class Camera{
    private:
        std::unique_ptr<GLSkybox> skybox;
        std::string currentProgram;
        std::unordered_map<std::string,ShaderProgram> programs;

        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 modelMatrix = glm::mat4(1.0f);

        glm::vec3 position = glm::vec3(0,0,0);
        glm::vec3 direction = glm::vec3(1,0,0);
        glm::vec3 up = glm::vec3(0,1,0);

        float screenWidth = 1920;
        float screenHeight = 1080;

    public:
        Camera();
        void resizeScreen(int width, int height, float FOV);
        void adjustFOV(float FOV);
        void updateUniforms();

        void setModelPosition(float x, float y, float z);
        void setPosition(float x, float y, float z);
        void setRotation(float pitch, float yaw);

        void addShader(std::string name, std::string vertex, std::string fragment);
        void addSkybox(std::string vertex, std::string fragment, std::array<std::string,6> paths);

        ShaderProgram& getProgram(std::string name);
        void useProgram(std::string name);

        glm::vec3& getDirection() {return direction;}
        void drawSkybox();
};

#endif