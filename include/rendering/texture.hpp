#ifndef TEXTURE_3D_H
#define TEXTURE_3D_H

#include <glad/glad.h>
#include <vector>
#include <string>
#include <iostream>
#include <array>

class BindableTexture{
    protected: 
        uint32_t texture;
        uint32_t TYPE;
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
        uint32_t getID() {return texture;}
};

class GLTexture: public BindableTexture{
    private:
        void loadData(unsigned char* data, int width, int height, int channels);

    public:
        GLTexture(const char* filename);
        GLTexture(unsigned char* data, int width, int height);
        GLTexture(){TYPE = GL_TEXTURE_2D;};
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
        uint32_t vertexBuffer;
        uint32_t vao;
    public:
        GLSkybox(std::array<std::string, 6> filenames);
        ~GLSkybox();

        void draw();
};

#include <rendering/buffer.hpp>

#endif