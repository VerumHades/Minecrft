#ifndef SHADERS_H
#define SHADERS_H

#include <glad/glad.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>  
#include <rendering/opengl/buffer.hpp>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <parsing/shader_parser.hpp>

#define MAX_SHADERS 16

class ShaderProgram;
template <typename T>
class Uniform;

class UniformBase{
    public:
        virtual void update(uint location) = 0;
        virtual std::string getName() = 0;
};

class ShaderUniformLinker{
    private:
        std::unordered_set<std::string> ignored_uniforms; // Usually uniforms reserver for texture bindings
        struct LinkedProgram{
            // Uniforms and their location in the program
            std::unordered_map<std::string, size_t> uniforms;
        };

        std::unordered_map<ShaderProgram*, LinkedProgram> shaderPrograms;
        std::unordered_map<std::string, UniformBase*> uniforms;

        void updateUniforms(ShaderProgram* program);

        void addUniform(UniformBase* uniform);
        void addProgram(ShaderProgram* program);

        void removeProgram(ShaderProgram* program);
        void removeUniform(UniformBase* uniform);

        friend class ShaderProgram;
        template <typename T>
        friend class Uniform;
    public:
        void ignore(std::string name){
            ignored_uniforms.emplace(name);
        }
};

extern ShaderUniformLinker uniformLinker;

static int programInUse = -1;
class ShaderProgram{
    private:
        int program = -1;
        std::vector<int> shaders = {};

    public:
        ShaderProgram(){
            this->program = glCreateProgram();
        }
        ~ShaderProgram(){
            glDeleteProgram(this->program);
            uniformLinker.removeProgram(this);
        }
        ShaderProgram(std::string vertex_shader_path, std::string fragment_shader_path): ShaderProgram() {
            addShader(vertex_shader_path, GL_VERTEX_SHADER);
            addShader(fragment_shader_path, GL_FRAGMENT_SHADER);
            compile();
        }
        ShaderProgram(std::string compute_shader_path): ShaderProgram() {
            addShader(compute_shader_path, GL_COMPUTE_SHADER);
            compile();
        }
        ShaderProgram(ShaderProgramSource shader_source): ShaderProgram() {
            for(auto& source: shader_source.getSources()) addShaderSource(source.source, source.shader_type);
            compile();
        }

        void setSamplerSlot(std::string name, int slot){
            use();
            int location = getUniformLocation(name);
            if(location == -1){
                std::cerr << "No sample under name '" << name << "' found." << std::endl;
                return;
            }
            uniformLinker.ignore(name);
            glUniform1i(location,slot);
        }
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

        int getUniformLocation(std::string name);

        int getID() {return program;};
};

template <typename T>
class Uniform: public UniformBase{
    private:
        T value;
        std::string name;

    public:
        Uniform(const std::string& uniformName){
            this->name = uniformName;
            uniformLinker.addUniform(reinterpret_cast<UniformBase*>(this));
        };
        ~Uniform(){
            uniformLinker.removeUniform(reinterpret_cast<UniformBase*>(this));
        }

        T& operator=(T newValue) {
            value = newValue; 
            return value;
        }

        void setValue(const T& newValue) {
            value = newValue;
        }

        T& getValue(){
            return value;
        }

        void update(uint location){
            //std::cout << "Updating uniform: " << name << " at: " << programID << std::endl;
            setUniformValue(value,  location);
        }

        std::string getName() {return name; };
    private:
        void setUniformValue(const glm::mat4& mat, int32_t location) {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
        }

        void setUniformValue(const glm::vec3& vec, int32_t location) {
            glUniform3fv(location, 1, glm::value_ptr(vec));
        }

        void setUniformValue(const std::vector<glm::vec3>& vectors, int32_t location){
            glUniform3fv(location, static_cast<GLsizei>(vectors.size()), glm::value_ptr(vectors[0]));
        }

        void setUniformValue(const std::vector<glm::mat4>& mats, int32_t location){
            glUniformMatrix4fv(location, static_cast<GLsizei>(mats.size()), GL_FALSE, glm::value_ptr(mats[0]));
        }

        void setUniformValue(const std::vector<glm::mat3>& mats, int32_t location){
            glUniformMatrix3fv(location, static_cast<GLsizei>(mats.size()), GL_FALSE, glm::value_ptr(mats[0]));
        }
};

#endif