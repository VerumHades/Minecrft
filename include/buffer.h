#ifndef BUFFER_H
#define BUFFER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <mesh.h>

void checkGLError(const char *file, int line);
#define CHECK_GL_ERROR() checkGLError(__FILE__, __LINE__)

typedef struct GLBuffer{
    unsigned int data;
    unsigned int index;
    unsigned int vao;

    unsigned int vertexCount;
    unsigned int indiciesCount;
} GLBuffer;

GLBuffer newBuffer();
void destroyBuffer(GLBuffer buffer);
void loadMeshToBuffer(Mesh* mesh, GLBuffer* glbuffer);
void drawBuffer(GLBuffer* glbuffer);

#endif