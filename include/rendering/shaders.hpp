#ifndef SHADERS_H
#define SHADERS_H

#include <glad/glad.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>  
#include <rendering/buffer.hpp>
#include <fstream>
#include <sstream>

#define MAX_SHADERS 16

class ShaderProgram{
    private:
        int program;
        std::vector<int> shaders;

        int projLoc = -1;
        int viewLoc = -1;
        int modelLoc = -1;

        bool isSkybox_ = false;
    public:
        void initialize();
        ~ShaderProgram();

        void addShader(std::string filename, int type);
        void compile();
        void use(){
            glUseProgram(this->program);
            CHECK_GL_ERROR();
        }

        int getProjLoc() {return projLoc;};
        int getViewLoc() {return viewLoc;};
        int getModelLoc() {return modelLoc;};

        bool isSkybox(){return isSkybox_; }; 
        void makeSkybox(){this->isSkybox_ = true;};

        int getID() {return program;};
};
#endif