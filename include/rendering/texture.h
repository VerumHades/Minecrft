#ifndef TEXTURE_3D_H
#define TEXTURE_3D_H

#include <glad/glad.h>

typedef struct GLTexture{
    unsigned int texture;
} GLTexture;

typedef struct GLTextureArray{
    unsigned int textureArray;
    unsigned int program;
} GLTextureArray;

typedef struct GLTexture3D{
    unsigned int texture;
    unsigned int program;
} GLTexture3D;

typedef struct GLSkybox{
    unsigned int texture;
    unsigned int vertexBuffer;
    unsigned int vao;
} GLSkybox;

#include <rendering/shaders.h>
#include <rendering/buffer.h>

GLTexture createTexture(ShaderProgram* program, char* filename);
void bindTexture(GLTexture* texture);
void destroyTexture(GLTexture texture);

GLTextureArray createTextureArray(ShaderProgram* program);
void loadTextureArrayFromFiles(GLTextureArray* tex, char* layers[], int layerCount, int layerWidth, int layerHeight);
void bindTextureArray(GLTextureArray* array);
void destroyTextureArray(GLTextureArray* array);

GLTexture3D createTexture3D(ShaderProgram* program);
void loadTexture3DRGB(GLTexture3D* texture, unsigned char data[], int layerWidth, int layerHeight, int layerCount);
void updateTexture3DRGB(GLTextureArray* tex, unsigned char data[], int layerWidth, int layerHeight, int layerCount);
void bindTexture3D(GLTexture3D* texture);
void destroyTexture3D(GLTexture3D* texture);

GLSkybox createSkybox(char* faces[], int facesTotal);
void drawSkybox(GLSkybox* skybox);
void destroySkybox(GLSkybox* skybox);

#endif