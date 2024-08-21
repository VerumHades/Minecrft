#include <rendering/shaders.hpp>

unsigned int compileShader(const char* source, int type){
    unsigned int shader = glCreateShader(type);
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

        printf("Error when compiling shader:\n%s", errorLog);
        free(errorLog);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.
        return -1;
    }

    return shader;
}


void ShaderProgram::initialize(){
    this->program = glCreateProgram();
}
ShaderProgram::~ShaderProgram(){
    glDeleteProgram(this->program);
}

glm::vec3& ShaderProgram::getCameraDirection(){
    return this->cameraDirection;
}

void ShaderProgram::addShader(char* filename, int type){
    char* source = readFilename(filename);
    if(source == NULL){
        printf("Failed to read shader source file '%s'\n", filename);
        exit(-1);
        return;
    }

    unsigned int shader = compileShader(source, type);
    this->shaders.push_back(shader);
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
}

void ShaderProgram::use(){
    glUseProgram(this->program);
}

