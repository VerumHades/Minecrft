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
        char* errorLog = calloc(maxLength, sizeof(char));
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

ShaderProgram newShaderProgram(){
    ShaderProgram program;

    program.program = glCreateProgram();
    program.shaderCount = 0;

    return program;
}

void addVertexShader(ShaderProgram* program, char* filename){
    char* source = readFilename(filename);
    if(source == NULL){
        printf("Failed to read shader source file '%s'\n", filename);
        exit(-1);
        return;
    }

    unsigned int shader = compileShader(source, GL_VERTEX_SHADER);
    program->shaders[program->shaderCount++] = shader;
}
void addFragmentShader(ShaderProgram* program, char* filename){
    char* source = readFilename(filename);
    if(source == NULL){
        printf("Failed to read shader source file '%s'\n", filename);
        exit(-1);
        return;
    }

    unsigned int shader = compileShader(source, GL_FRAGMENT_SHADER);
    program->shaders[program->shaderCount++] = shader;
}

void compileShaderProgram(ShaderProgram* program){
    for(int i = 0;i < program->shaderCount;i++){
        glAttachShader(program->program, program->shaders[i]);
    }

    glLinkProgram(program->program);
    glUseProgram(program->program);
    
    for(int i = 0;i < program->shaderCount;i++){
        glDeleteShader(program->shaders[i]);
    }
}

void useShaderProgram(ShaderProgram* program){
    glUseProgram(program->program);
}

