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

        unsigned int vertexCount = 0;
        unsigned int indiciesCount = 0;

        bool dataLoaded = false;
    public:
        GLBuffer();
        ~GLBuffer();
        void loadMesh(Mesh& mesh);
        void draw();
};

class GLDoubleBuffer{
    private:
        GLBuffer buffers[2];
        int current = 0;
    public:
        void swap();
        GLBuffer& getBuffer();
        GLBuffer& getBackBuffer();
};

#include <rendering/mesh.hpp>

#endif