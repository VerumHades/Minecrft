#ifndef TEXTURE_3D_H
#define TEXTURE_3D_H

#include <glad/glad.h>
#include <vector>
#include <string>
#include <iostream>
#include <array>

class BindableTexture{
    protected: 
        unsigned int texture;
        unsigned int TYPE;
        BindableTexture(){
            glGenTextures(1, &this->texture);
        }
        ~BindableTexture(){
            glDeleteTextures(1, &this->texture);
        }
    public:
        void bind(int unit){
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(TYPE, this->texture);
        }  
        unsigned int getID() {return texture;}
};

class GLTexture: public BindableTexture{
    public:
        GLTexture(char* filename);
};

class GLDepthTexture: public BindableTexture{
    private:
        int width;
        int height;

    public:
        GLDepthTexture(int width, int height);

        int getWidth(){return width;}
        int getHeight(){return height;}
};

class GLTextureArray: public BindableTexture{
    public:
        void loadFromFiles(std::vector<std::string> filenames, int layerWidth, int layerHeight);
};

class GLSkybox: public BindableTexture{
    private:
        unsigned int vertexBuffer;
        unsigned int vao;
    public:
        GLSkybox(std::array<std::string, 6> filenames);
        ~GLSkybox();

        void draw();
};

#include <rendering/buffer.hpp>

#endif