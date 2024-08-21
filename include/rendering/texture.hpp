#ifndef TEXTURE_3D_H
#define TEXTURE_3D_H

#include <glad/glad.h>
#include <vector>
#include <string>
#include <iostream>
#include <array>

class GLTexture{
    private:
        unsigned int texture;
    public:
        GLTexture(char* filename);
        ~GLTexture();
        void bind();
};

class GLTextureArray{
    private:
        unsigned int textureArray;
    public:
        GLTextureArray();
        ~GLTextureArray();

        void loadFromFiles(std::vector<std::string> filenames, int layerWidth, int layerHeight);
        void bind();
};

class GLSkybox{
    private:
        unsigned int texture;
        unsigned int vertexBuffer;
        unsigned int vao;
    public:
        GLSkybox(std::array<std::string, 6> filenames);
        ~GLSkybox();

        void draw();
};

#include <rendering/shaders.hpp>
#include <rendering/buffer.hpp>

#endif