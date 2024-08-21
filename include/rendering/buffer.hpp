#ifndef BUFFER_H
#define BUFFER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

void checkGLError(const char *file, int line);
#define CHECK_GL_ERROR() checkGLError(__FILE__, __LINE__)
class Mesh;

class GLBuffer{
    private:
        unsigned int data;
        unsigned int index;
        unsigned int vao;

        unsigned int vertexCount;
        unsigned int indiciesCount;
    public:
        GLBuffer();
        ~GLBuffer();
        void loadMesh(Mesh& mesh);
        void draw();
};

#include <rendering/mesh.hpp>

#endif