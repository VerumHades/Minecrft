#include <rendering/shaders.hpp>

uint32_t compileShader(const char* source, int type){
    uint32_t shader = glCreateShader(type);
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

        std::cout << "Error when compiling shader:" << errorLog << std::endl;
        free(errorLog);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.
        throw std::runtime_error("");
    }

    return shader;
}


void ShaderProgram::initialize(){
    this->program = glCreateProgram();
}
ShaderProgram::~ShaderProgram(){
    glDeleteProgram(this->program);
}

void ShaderProgram::addShader(std::string filename, int type){
    std::ifstream file(filename);  // Open the file
    if (!file.is_open()) {              // Check if the file is open
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();             

    std::string source = buffer.str();

    file.close();  // Close the file

    uint32_t shader = compileShader(source.c_str(), type);
    this->shaders.push_back(shader);
}

void ShaderProgram::addShaderSource(std::string source, int type){
    uint32_t shader = compileShader(source.c_str(), type);
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

    this->projLoc = glGetUniformLocation(this->program, "projection");
    this->viewLoc = glGetUniformLocation(this->program, "view");
    this->modelLoc = glGetUniformLocation(this->program, "model");
}

void ShaderProgram::updateUniforms(){
    this->use();
    for (auto& [key, value]: attachedUniforms) {
        value.get().update(this->program);
    }
}

void ShaderProgram::updateUniform(std::string name){
    this->use();
    attachedUniforms.at(name).get().update(this->program);
}