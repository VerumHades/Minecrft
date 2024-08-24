#ifndef SHADERS_H
#define SHADERS_H

#include <glad/glad.h>
#include <iostream>
#include <cstdlib>
#include <standard.hpp>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>  
#include <rendering/buffer.hpp>

#define MAX_SHADERS 16

class ShaderProgram{
    private:
        unsigned int program;
        std::vector<unsigned int> shaders;

        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 modelMatrix;

        glm::vec3 cameraPosition = glm::vec3(0,0,0);
        glm::vec3 cameraDirection = glm::vec3(0,0,0);
        glm::vec3 cameraUp = glm::vec3(0,1,0);

        unsigned int projLoc = -1;
        unsigned int viewLoc = -1;
        unsigned int modelLoc = -1;

        bool projectionSetup = false;
        bool isSkybox = false;

        void setupProjection(int width, int height, float FOV);
        void updateViewMatrix();

    public:
        void initialize();
        ~ShaderProgram();

        glm::vec3& getCameraDirection();

        void addShader(char* filename, int type);
        void compile();
        void use();

        void recalculateProjection(int width, int height, float FOV);
        void setModelPosition(float x, float y, float z);
        void setCameraPosition(float x, float y, float z);
        void setCameraRotation(float pitch, float yaw);
        void makeSkybox();

};
#endif