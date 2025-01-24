#include <rendering/opengl/shaders.hpp>

uint compileShader(const char* source, int type, std::string filename = ""){
    uint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if(isCompiled == GL_FALSE)
    {
        int maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        char* errorLog = (char*) calloc(maxLength, sizeof(char));
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

        std::cout << "Error when compiling shader:" << errorLog << ((filename != "") ? "In file: " + filename : "") << std::endl;
        free(errorLog);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.
        throw std::runtime_error("");
    }

    return shader;
}

std::string ShaderProgram::getSource(const std::string& path){
    std::ifstream file(path);  // Open the file
    if (!file.is_open()) {              // Check if the file is open
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }
         

    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return source;
}

void ShaderProgram::addShader(std::string filename, int type){
    uint shader = compileShader(getSource(filename).c_str(), type, filename);
    this->shaders.push_back(shader);
}

void ShaderProgram::addShaderSource(std::string source, int type){
    uint shader = compileShader(source.c_str(), type);
    this->shaders.push_back(shader);
}

int ShaderProgram::getUniformLocation(std::string name){
    this->use();
    return glGetUniformLocation(this->program, name.c_str());
}

void ShaderProgram::compile(){
    for(int i = 0;i < this->shaders.size();i++){
        glAttachShader(this->program, this->shaders[i]);
    }

    glLinkProgram(this->program);
    glUseProgram(this->program);
    
    for(int i = 0;i < this->shaders.size();i++){
        glDeleteShader(this->shaders[i]);
    }

    ShaderUniformLinker::get().addProgram(this);
}

void ShaderProgram::updateUniforms(){
    ShaderUniformLinker::get().updateUniforms(this);
}

ShaderUniformLinker& ShaderUniformLinker::get(){
    static ShaderUniformLinker linker;
    return linker;
}

void ShaderUniformLinker::updateUniforms(ShaderProgram* program){
    if(!shaderPrograms.contains(program)) return;

    program->use();
    for(auto& [name,location]: shaderPrograms[program].uniforms){
        if(!uniforms.contains(name)){
            if(ignored_uniforms.contains(name)) continue;
            //std::cerr << "Shader program is missing a uniform: " << name << std::endl;
            continue;
        }

        uniforms[name]->update(location);
    }
}

void ShaderUniformLinker::addProgram(ShaderProgram* program){
    LinkedProgram linked_program = {};
    
    program->use();
    GLint numUniforms = 0;
    glGetProgramiv(program->getID(), GL_ACTIVE_UNIFORMS, &numUniforms);

    for (GLint i = 0; i < numUniforms; ++i) {
        char name_buffer[256]; 
        GLsizei nameLength = 0;
        GLint size = 0;
        GLenum type = 0;

        glGetActiveUniform(program->getID(), i, sizeof(name_buffer), &nameLength, &size, &type, name_buffer);

        std::string name = std::string(name_buffer);
        if(name.ends_with("]")){ // For array uniforms
            name = name.substr(0, name.size() - 3);
            std::cout << "Changed name: " << name <<  std::endl;
        }

        int location = glGetUniformLocation(program->getID(), name.c_str());

        if(location == -1) {
            std::cerr << "Reported uniform '" << name << "' not found." << std::endl;
            continue;
        }

        linked_program.uniforms[name] = location;
    }

    shaderPrograms[program] = linked_program;
}

void ShaderUniformLinker::addUniform(UniformBase* uniform){
    if(uniforms.contains(uniform->getName())){
        std::cerr << "Cannot overwrite already existing uniform '" << uniform->getName() << "'." << std::endl;
        return;
    }
    //std::cout << "Added uniform '" << uniform->getName() << "' to linker." << std::endl;
    uniforms[uniform->getName()] = uniform;
}

void ShaderUniformLinker::removeProgram(ShaderProgram* program){
    if(!shaderPrograms.contains(program)){ // Can happend for uncompiled programs
        return;
    }

    shaderPrograms.erase(program);
}
void ShaderUniformLinker::removeUniform(UniformBase* uniform){
    if(!uniforms.contains(uniform->getName())){
        std::cerr << "Removing a missing uniform from uniform linker? This shouldnt happen." << std::endl;
        return;
    }

    uniforms.erase(uniform->getName());
}