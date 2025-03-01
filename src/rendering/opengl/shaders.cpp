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

    auto& linkable_program = shaderPrograms[program];
    if(linkable_program.to_be_linked.size() > 0) 
        linkUniforms(linkable_program);

    program->use();
    for(auto& [location,index]: linkable_program.linked)
        uniform_ptr_list.at(index)->update(location);
}

void ShaderUniformLinker::linkUniforms(LinkedProgram& program){
    for (auto it = program.to_be_linked.begin(); it != program.to_be_linked.end(); ) {
        auto& [name, location] = *it;
        if(!uniforms.contains(name)) ++it;
        else{
            program.linked.push_back({location, uniforms[name].list_index});
            it = program.to_be_linked.erase(it);
        }
    }
}

void ShaderUniformLinker::addProgram(ShaderProgram* program){
    LinkedProgram linked_program = {};
    
    program->use();
    GLint numUniforms = 0;
    glGetProgramiv(program->getID(), GL_ACTIVE_UNIFORMS, &numUniforms);

    linked_program.to_be_linked.reserve(numUniforms);
    linked_program.linked.reserve(numUniforms);
    
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

        linked_program.to_be_linked.push_back({name, location});
    }

    shaderPrograms[program] = linked_program;
}


void ShaderUniformLinker::removeProgram(ShaderProgram* program){
    if(!shaderPrograms.contains(program)){ // Can happend for uncompiled programs
        return;
    }

    shaderPrograms.erase(program);
}
