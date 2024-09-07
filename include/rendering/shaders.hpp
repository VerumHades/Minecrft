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

class UniformBase{
    public:
        virtual void update(uint32_t programID) = 0;
        virtual std::string getName() = 0;
};

static int programInUse = -1;
class ShaderProgram{
    private:
        int program;
        std::vector<int> shaders;
        std::unordered_map<std::string, std::reference_wrapper<UniformBase>> attachedUniforms;

        int projLoc = -1;
        int viewLoc = -1;
        int modelLoc = -1;

        bool isSkybox_ = false;
    public:
        void initialize();
        ~ShaderProgram();

        void addShader(std::string filename, int type);
        void addShaderSource(std::string source, int type);
        void compile();
        void use(){
            if(programInUse == program) return;
            programInUse = program;
            //if(!glIsProgram(this->program)) std::cout << "Invalid program?" << std::endl;
            glUseProgram(this->program);
        }
        void updateUniforms();
        void updateUniform(std::string name);

        int getProjLoc() {return projLoc;};
        int getViewLoc() {return viewLoc;};
        int getModelLoc() {return modelLoc;};

        int getUniformLocation(std::string name);

        bool isSkybox(){return isSkybox_; }; 
        void makeSkybox(){this->isSkybox_ = true;};

        int getID() {return program;};
        void attachUniform(UniformBase& uniform) {attachedUniforms.emplace(uniform.getName(), uniform);}
};

template <typename T>
class Uniform: public UniformBase{
    private:
        T value;
        std::unordered_map<int32_t, int32_t> locations;
        std::string name;

    public:
        Uniform(const std::string& uniformName) : name(uniformName) {
            
        };

        T& operator=(T newValue) {
            value = newValue; 
            return value;
        }

        void attach(ShaderProgram& program){
            locations[program.getID()] = program.getUniformLocation(name);
            if(locations[program.getID()] == -1) {
                std::cout << "Failed to get uniform: " << name << " from program: " << program.getID() << std::endl;
                locations.erase(program.getID());
                return;
            }
            program.attachUniform(*this);
        }

        void setValue(const T& newValue) {
            value = newValue;
        }

        T& getValue(){
            return value;
        }

        void update(uint32_t programID){
            //std::cout << "Updating uniform: " << name << " at: " << programID << std::endl;
            setUniformValue(value, locations[programID]);
        }

        std::string getName() {return name; };
    private:
        void setUniformValue(const glm::mat4& mat, int32_t location) {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
        }

        void setUniformValue(const glm::vec3& vec, int32_t location) {
            glUniform3fv(location, 1, glm::value_ptr(vec));
        }

        void setUniformValue(const std::vector<glm::mat4>& mats, int32_t location){
            glUniformMatrix4fv(location, static_cast<GLsizei>(mats.size()), GL_FALSE, glm::value_ptr(mats[0]));
        }

        void setUniformValue(const std::vector<glm::mat3>& mats, int32_t location){
            glUniformMatrix3fv(location, static_cast<GLsizei>(mats.size()), GL_FALSE, glm::value_ptr(mats[0]));
        }
};

#endif