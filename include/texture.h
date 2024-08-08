#ifndef TEXTURE_3D_H
#define TEXTURE_3D_H

#include <glad/glad.h>
#include <shaders.h>
#include <buffer.h>

typedef struct GLTexture{
    unsigned int texture;
} GLTexture;

typedef struct GLTextureArray{
    unsigned int textureArray;
} GLTextureArray;

typedef struct GLSkybox{
    unsigned int texture;
    unsigned int vertexBuffer;
    unsigned int vao;
} GLSkybox;

GLTexture createTexture(ShaderProgram* program, char* filename);
void bindTexture(GLTexture* texture);
void destroyTexture(GLTexture texture);

GLTextureArray createTextureArray(ShaderProgram* program, char* layers[], int layerCount, int layerWidth, int layerHeight);
void bindTextureArray(GLTextureArray* array);
void destroyTextureArray(GLTextureArray* array);

GLSkybox createSkybox(char* faces[], int facesTotal);
void drawSkybox(GLSkybox* skybox);
void destroySkybox(GLSkybox* skybox);

#endif