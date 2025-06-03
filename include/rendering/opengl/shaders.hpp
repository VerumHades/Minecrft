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
#include <memory>

class ShaderProgram;
class ShaderUniformLinker;
template <typename T>
class Uniform;

class UniformBase{
    public:
        virtual void update(uint location) = 0;
        virtual std::string getName() = 0;
};

/**
 * @brief An actual uniform instance
 * 
 * @tparam T 
 */
template <typename T>
class FunctionalUniform: public UniformBase{
    private:
        T value;
        std::string name;

        friend class ShaderUniformLinker;
        template <typename L>
        friend class Uniform;
    public:
        FunctionalUniform(const std::string& uniformName){
            this->name = uniformName;
        };

        T& operator=(T newValue) {
            value = newValue; 
            return value;
        }

        void setValue(const T& newValue) {
            value = newValue;
        }

        const T& getValue() const {
            return value;
        }

        void update(uint location){
            setUniformValue(value,  location);
        }

        std::string getName() {return name; };
    private:
        void setUniformValue(const glm::mat4& mat, int32_t location) {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
        }

        void setUniformValue(const float& vec, int32_t location) {
            glUniform1f(location, vec);
        }

        void setUniformValue(const glm::vec3& vec, int32_t location) {
            glUniform3fv(location, 1, glm::value_ptr(vec));
        }

        void setUniformValue(const glm::ivec3& vec, int32_t location) {
            glUniform3iv(location, 1, glm::value_ptr(vec));
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

/**
 * @brief A background class that manages linking of uniforms to shaders that use them
 * 
 */
class ShaderUniformLinker{
    private:
        std::unordered_set<std::string> ignored_uniforms; // Usually uniforms reserver for texture bindings

        struct LinkedProgram{
            // Uniforms and their location in the program
            std::vector<std::tuple<std::string, size_t>> to_be_linked;
            // Location and index in the uniform_ptr_list;
            std::vector<std::tuple<size_t, size_t>> linked;
        };

        struct RegisteredUniform{
            size_t list_index;
            std::unique_ptr<UniformBase> uniform;
        };

        std::unordered_map<ShaderProgram*, LinkedProgram> shaderPrograms;
        
        std::unordered_map<std::string, RegisteredUniform> uniforms;
        std::vector<UniformBase*> uniform_ptr_list;

        void linkUniforms(LinkedProgram& program);

        void updateUniforms(ShaderProgram* program);

        void addProgram(ShaderProgram* program);
        void removeProgram(ShaderProgram* program);

        friend class ShaderProgram;
        template <typename T>
        friend class Uniform;

    public:
        /*
            Returns a reference to a uniform of set type, creates one under the name if it doesnt exist
        */
        template <typename T>
        static FunctionalUniform<T>* getUniform(const std::string& name){
            if(get().uniforms.contains(name)){
                FunctionalUniform<T>* ptr = dynamic_cast<FunctionalUniform<T>*>(get().uniforms.at(name).uniform.get());
                if(!ptr){
                    LogError("Existing uniform '{}' has different type that the one fetched.", name);
                    return nullptr;
                }
                return ptr;
            }

            RegisteredUniform new_uniform = {
                get().uniform_ptr_list.size(),
                std::make_unique<FunctionalUniform<T>>(name)
            };
            auto* value = static_cast<FunctionalUniform<T>*>(new_uniform.uniform.get());

            get().uniform_ptr_list.push_back(value);
            get().uniforms.emplace(name,std::move(new_uniform));

            return value;
        }

        void ignore(std::string name){
            ignored_uniforms.emplace(name);
        }

        static ShaderUniformLinker& get();
};

/**
 * @brief A reference to a uniform instance that one can manipulate freely
 * 
 * @tparam T 
 */
template <typename T>
class Uniform{
    private:
        FunctionalUniform<T>* base;

    public:
        Uniform(const std::string& name){
            base = ShaderUniformLinker::getUniform<T>(name);
            if(!base) throw std::runtime_error("Uniform cannot operate without base, check for type errors.");
        }

        T& operator=(T newValue) {
            base->value = newValue; 
            return base->value;
        }

        void setValue(const T& newValue) {
            base->value = newValue;
        }

        T& getValue(){
            return base->value;
        }

        const T& getValue() const {
            return base->value;
        }

        void update(uint location){
            base->setUniformValue(base->value, location);
        }
};

/**
 * @brief A shader program wrapper
 * 
 */
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
            if(this->program != -1) {GL_CALL( glDeleteProgram(this->program))};
            ShaderUniformLinker::get().removeProgram(this);
        }

        ShaderProgram(const ShaderProgram& other) = delete;
        ShaderProgram& operator=(const ShaderProgram& other) = delete;

        ShaderProgram(ShaderProgram&& other) noexcept {
            program = other.program;
            other.program = -1;
        }

        ShaderProgram& operator=(ShaderProgram&& other) noexcept {
            if (this != &other) {
                if(this->program != -1) {GL_CALL( glDeleteProgram(this->program))};
                program = other.program;
                other.program = -1;
            }
            return *this;
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

        static std::string getSource(const std::string& path);
        
        void setSamplerSlot(std::string name, int slot){
            use();
            int location = getUniformLocation(name);
            if(location == -1){
                LogError("No sampler under name '{}' found.", name);
                return;
            }
            ShaderUniformLinker::get().ignore(name);
            glUniform1i(location,slot);
        }
        void addShader(std::string filename, int type);
        void addShaderSource(std::string source, int type);
        void compile();
        void use(){
            if(programInUse == program) return;
            programInUse = program;
            GL_CALL( glUseProgram(this->program));
        }
        void updateUniforms();

        int getUniformLocation(std::string name);

        int getID() {return program;};
};

#endif