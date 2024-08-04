#ifndef SHADERS_H
#define SHADERS_H

#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <standard.h>

#define MAX_SHADERS 16

typedef struct ShaderProgram{
    unsigned int program;
    unsigned int shaders[MAX_SHADERS];
    unsigned int shaderCount;

    float projectionMatrix[16];
    float viewMatrix[16];
    float modelMatrix[16];

    unsigned int projLoc;
    unsigned int viewLoc;
    unsigned int modelLoc;
} ShaderProgram;

ShaderProgram newShaderProgram();
void addVertexShader(ShaderProgram* program, char* filename);
void addFragmentShader(ShaderProgram* program, char* filename);
void compileShaderProgram(ShaderProgram* program);
void useShaderProgram(ShaderProgram* program);

#endif